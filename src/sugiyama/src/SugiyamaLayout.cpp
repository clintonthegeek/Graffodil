#include <graffodil/SugiyamaLayout.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>

#include <QHash>
#include <QList>
#include <QQueue>
#include <QSet>
#include <QtMath>
#include <algorithm>
#include <climits>
#include <functional>
#include <limits>

namespace Graffodil {

// ---- Helpers for the Sugiyama framework ----

// Internal topological sort using Kahn's algorithm.
static QList<QString> topologicalSort(
    const QList<QString> &allIds,
    const QHash<QString, QList<QString>> &successors,
    const QHash<QString, QList<QString>> &predecessors)
{
    QHash<QString, int> inDegree;
    for (const auto &id : allIds)
        inDegree[id] = predecessors.value(id).size();

    QQueue<QString> queue;
    for (const auto &id : allIds) {
        if (inDegree[id] == 0)
            queue.enqueue(id);
    }

    // Sort initial queue alphabetically for determinism
    QList<QString> initial;
    while (!queue.isEmpty())
        initial.append(queue.dequeue());
    std::sort(initial.begin(), initial.end());
    for (const auto &id : initial)
        queue.enqueue(id);

    QList<QString> result;
    while (!queue.isEmpty()) {
        QString cur = queue.dequeue();
        result.append(cur);
        QList<QString> succs = successors.value(cur);
        std::sort(succs.begin(), succs.end()); // alphabetical for determinism
        for (const auto &s : succs) {
            if (--inDegree[s] == 0)
                queue.enqueue(s);
        }
    }
    return result;
}

// Find connected components via BFS on an undirected adjacency list.
// Returns list of components, each component is a list of node IDs.
// Neighbors are visited in alphabetical order for determinism.
static QList<QList<QString>> findComponents(
    const QList<QString> &allIds,
    const QHash<QString, QSet<QString>> &undirectedAdj)
{
    QList<QList<QString>> components;
    QSet<QString> visited;

    for (const QString &uid : allIds) {
        if (visited.contains(uid))
            continue;

        QList<QString> component;
        QQueue<QString> queue;
        queue.enqueue(uid);
        visited.insert(uid);

        while (!queue.isEmpty()) {
            QString cur = queue.dequeue();
            component.append(cur);

            // Sort neighbors alphabetically for deterministic BFS
            QList<QString> neighbors = undirectedAdj.value(cur).values();
            std::sort(neighbors.begin(), neighbors.end());

            for (const QString &neighbor : neighbors) {
                if (!visited.contains(neighbor)) {
                    visited.insert(neighbor);
                    queue.enqueue(neighbor);
                }
            }
        }

        // Sort component members alphabetically
        std::sort(component.begin(), component.end());

        components.append(component);
    }

    // Sort components alphabetically by their first member
    std::sort(components.begin(), components.end(),
        [](const QList<QString> &a, const QList<QString> &b) {
            QString ai = a.isEmpty() ? QString() : a.first();
            QString bi = b.isEmpty() ? QString() : b.first();
            return ai < bi;
        });

    return components;
}

// Count edge crossings between all pairs of adjacent layers.
static int countCrossings(
    int maxLayer,
    const QHash<int, QList<QString>> &layerNodes,
    const QHash<QString, int> &nodePosition,
    const QHash<QString, QList<QString>> &successors)
{
    int total = 0;

    for (int l = 0; l < maxLayer; ++l) {
        QList<QPair<int, int>> edgePairs;
        const QList<QString> &curLayer = layerNodes.value(l);

        for (const QString &u : curLayer) {
            int uPos = nodePosition.value(u);
            for (const QString &v : successors.value(u)) {
                if (nodePosition.contains(v)) {
                    edgePairs.append({uPos, nodePosition.value(v)});
                }
            }
        }

        for (int i = 0; i < edgePairs.size(); ++i) {
            for (int j = i + 1; j < edgePairs.size(); ++j) {
                int a = edgePairs[i].first, b = edgePairs[i].second;
                int c = edgePairs[j].first, d = edgePairs[j].second;
                if ((a < c && b > d) || (a > c && b < d))
                    ++total;
            }
        }
    }
    return total;
}

// Snapshot the ordering: for each layer, record the list of IDs in order.
static QHash<int, QList<QString>> snapshotOrdering(
    int maxLayer,
    const QHash<int, QList<QString>> &layerNodes)
{
    QHash<int, QList<QString>> snapshot;
    for (int l = 0; l <= maxLayer; ++l)
        snapshot[l] = layerNodes.value(l);
    return snapshot;
}

// Rebuild nodePosition map from layerNodes ordering.
static QHash<QString, int> buildPositionMap(
    int maxLayer,
    const QHash<int, QList<QString>> &layerNodes)
{
    QHash<QString, int> pos;
    for (int l = 0; l <= maxLayer; ++l) {
        const QList<QString> &nodes = layerNodes.value(l);
        for (int i = 0; i < nodes.size(); ++i)
            pos[nodes[i]] = i;
    }
    return pos;
}

// Brandes-Kopf compact Y-coordinate assignment.
// Produces tighter layouts than grid-based positioning by aligning
// dummy vertices vertically, making long edges nearly straight.
// Four "extreme" layouts (forward/backward x top/bottom preference),
// each greedily aligning nodes with their median neighbor; final
// coordinate is the median of four. Linear time per pass.
static QHash<QString, qreal> brandesKopfYCoordinates(
    int maxLayer,
    const QHash<int, QList<QString>> &layerNodes,
    const QHash<QString, int> &layerMap,
    const QHash<QString, int> &nodePosition,
    const QHash<QString, QList<QString>> &pred,
    const QHash<QString, QList<QString>> &succ,
    qreal minSep)
{
    // Collect all node IDs across all layers
    QList<QString> allIds;
    for (int l = 0; l <= maxLayer; ++l)
        allIds.append(layerNodes.value(l));

    QHash<QString, qreal> extremeY[4];

    for (int dir = 0; dir < 4; ++dir) {
        bool forward = (dir < 2);
        bool topPref = (dir % 2 == 0);

        // ---- Vertical alignment ----
        QHash<QString, QString> root, alignNext;
        for (const QString &v : allIds) {
            root[v] = v;
            alignNext[v] = v;
        }

        int lStart = forward ? 1 : maxLayer - 1;
        int lEnd   = forward ? maxLayer + 1 : -1;
        int lStep  = forward ? 1 : -1;

        for (int l = lStart; l != lEnd; l += lStep) {
            const QList<QString> &nodes = layerNodes.value(l);
            int n = nodes.size();
            int r = topPref ? -1 : n;

            int kStart = topPref ? 0 : n - 1;
            int kEnd   = topPref ? n : -1;
            int kStep  = topPref ? 1 : -1;

            for (int k = kStart; k != kEnd; k += kStep) {
                const QString &v = nodes[k];
                const QList<QString> &nbrs =
                    forward ? pred.value(v) : succ.value(v);

                QList<QPair<int,QString>> sorted;
                for (const QString &u : nbrs)
                    if (nodePosition.contains(u))
                        sorted.append({nodePosition[u], u});
                std::sort(sorted.begin(), sorted.end());

                int d = sorted.size();
                if (d == 0) continue;

                int mL = (d - 1) / 2;
                int mR = d / 2;
                int mStart = topPref ? mL : mR;
                int mEnd   = topPref ? mR + 1 : mL - 1;
                int mStep  = topPref ? 1 : -1;

                for (int m = mStart; m != mEnd; m += mStep) {
                    if (alignNext[v] != v) break;
                    const QString &um = sorted[m].second;
                    int umPos = sorted[m].first;

                    bool valid = topPref ? (umPos > r) : (umPos < r);
                    if (valid) {
                        alignNext[um] = v;
                        root[v] = root[um];
                        alignNext[v] = root[v];
                        r = umPos;
                    }
                }
            }
        }

        // ---- Compaction: assign Y coordinates ----
        QHash<QString, qreal> y;
        QHash<QString, QString> sink;
        QHash<QString, qreal> shift;
        QSet<QString> placed;

        for (const QString &v : allIds) {
            sink[v] = v;
            shift[v] = std::numeric_limits<qreal>::max();
        }

        std::function<void(const QString &)> placeBlock =
            [&](const QString &v) {
            if (placed.contains(v)) return;
            placed.insert(v);
            y[v] = 0;

            QString w = v;
            do {
                int pos = nodePosition.value(w, -1);
                int wl  = layerMap.value(w, -1);
                const QList<QString> &lnodes = layerNodes.value(wl);

                int adjPos = topPref ? (pos - 1) : (pos + 1);
                bool hasAdj = topPref
                    ? (pos > 0)
                    : (pos < lnodes.size() - 1);

                if (hasAdj) {
                    const QString &adj = lnodes[adjPos];
                    const QString &u = root[adj];
                    placeBlock(u);

                    if (sink[v] == v) sink[v] = sink[u];

                    if (sink[v] != sink[u]) {
                        if (topPref)
                            shift[sink[u]] = qMin(shift[sink[u]],
                                                  y[v] - y[u] - minSep);
                        else
                            shift[sink[u]] = qMin(shift[sink[u]],
                                                  y[u] - y[v] - minSep);
                    } else {
                        if (topPref)
                            y[v] = qMax(y[v], y[u] + minSep);
                        else
                            y[v] = qMin(y[v], y[u] - minSep);
                    }
                }

                w = alignNext[w];
            } while (w != v);
        };

        for (const QString &v : allIds)
            placeBlock(root[v]);

        for (const QString &v : allIds)
            y[v] = y[root[v]];

        for (const QString &v : allIds) {
            qreal s = shift[sink[root[v]]];
            if (s < std::numeric_limits<qreal>::max()) {
                if (topPref)
                    y[v] += s;
                else
                    y[v] -= s;
            }
        }

        extremeY[dir] = y;
    }

    // ---- Combine: normalize each layout then take median ----
    for (int d = 0; d < 4; ++d) {
        qreal minVal = std::numeric_limits<qreal>::max();
        for (auto it = extremeY[d].begin(); it != extremeY[d].end(); ++it)
            minVal = qMin(minVal, it.value());
        for (auto it = extremeY[d].begin(); it != extremeY[d].end(); ++it)
            it.value() -= minVal;
    }

    QHash<QString, qreal> result;
    for (const QString &v : allIds) {
        qreal ys[4];
        for (int d = 0; d < 4; ++d)
            ys[d] = extremeY[d].value(v, 0);
        std::sort(ys, ys + 4);
        result[v] = (ys[1] + ys[2]) / 2.0;
    }
    return result;
}

// ---- SugiyamaLayout implementation ----

void SugiyamaLayout::setNodes(const QList<IGraphNode *> &nodes)
{
    m_nodes = nodes;
}

void SugiyamaLayout::setEdges(const QList<IGraphEdge *> &edges)
{
    m_edges = edges;
}

void SugiyamaLayout::setNodeSize(const QSizeF &size)
{
    m_nodeSize = size;
}

void SugiyamaLayout::setSpacing(qreal horizontal, qreal vertical)
{
    m_hSpacing = horizontal;
    m_vSpacing = vertical;
}

void SugiyamaLayout::setSpine(const QStringList &orderedNodeIds)
{
    m_spine = orderedNodeIds;
}

void SugiyamaLayout::setSpineAlignment(bool enabled)
{
    m_spineAlignment = enabled;
}

void SugiyamaLayout::setDirection(Direction d)
{
    m_direction = d;
}

SugiyamaLayout::Direction SugiyamaLayout::direction() const
{
    return m_direction;
}

LayoutResult SugiyamaLayout::compute()
{
    LayoutResult result;

    // Build allTasks from m_nodes
    QList<QString> allTasks;
    for (const auto *node : m_nodes)
        allTasks.append(node->nodeId());

    if (allTasks.isEmpty())
        return result;

    // Build alphabetical index for deterministic tie-breaking
    QList<QString> sortedIds = allTasks;
    std::sort(sortedIds.begin(), sortedIds.end());
    QHash<QString, int> alphaIndex;
    for (int i = 0; i < sortedIds.size(); ++i)
        alphaIndex[sortedIds[i]] = i;

    // Build edges from m_edges
    QList<QPair<QString, QString>> edges;
    for (const auto *edge : m_edges)
        edges.append({edge->sourceNode()->nodeId(), edge->targetNode()->nodeId()});

    // Build directed adjacency (predecessors and successors)
    QHash<QString, QList<QString>> predecessors;
    QHash<QString, QList<QString>> successors;
    for (const auto &edge : edges) {
        predecessors[edge.second].append(edge.first);
        successors[edge.first].append(edge.second);
    }

    // Build undirected adjacency for component detection
    QHash<QString, QSet<QString>> undirectedAdj;
    for (const QString &uid : allTasks)
        undirectedAdj[uid]; // ensure all nodes exist in map
    for (const auto &edge : edges) {
        undirectedAdj[edge.first].insert(edge.second);
        undirectedAdj[edge.second].insert(edge.first);
    }

    // Spine set for center-spine layout (only if alignment is enabled)
    QSet<QString> cpSet;
    if (m_spineAlignment) {
        cpSet = QSet<QString>(m_spine.begin(), m_spine.end());
    }

    // ---- Phase 1: Disconnected component detection ----
    QList<QList<QString>> components = findComponents(allTasks, undirectedAdj);

    // Topological order for use in initial ordering within layers
    QList<QString> topoOrder = topologicalSort(allTasks, successors, predecessors);
    if (topoOrder.isEmpty())
        topoOrder = allTasks;

    // Build topo-index for tiebreaking
    QHash<QString, int> topoIndex;
    for (int i = 0; i < topoOrder.size(); ++i)
        topoIndex[topoOrder[i]] = i;

    // Internally we compute positions in a (u, v) frame where u is the
    // layer axis and v is the within-layer axis, stored as QPointF(u, v).
    // The final (u, v) -> (x, y) transform is applied after all components
    // are laid out. For LeftToRight (default), u == x and v == y, and
    // this transform is the identity.
    const bool swapAxes = (m_direction == Direction::TopToBottom
                        || m_direction == Direction::BottomToTop);
    const bool mirror   = (m_direction == Direction::RightToLeft
                        || m_direction == Direction::BottomToTop);

    const qreal layerExtent  = swapAxes ? m_nodeSize.height() : m_nodeSize.width();
    const qreal withinExtent = swapAxes ? m_nodeSize.width()  : m_nodeSize.height();
    const qreal layerGap     = swapAxes ? m_vSpacing          : m_hSpacing;
    const qreal withinGap    = swapAxes ? m_hSpacing          : m_vSpacing;

    qreal componentYOffset = 0;

    // Process each component independently
    for (const QList<QString> &component : components) {
        if (component.isEmpty())
            continue;

        QSet<QString> compSet(component.begin(), component.end());

        // ---- Phase 2: Longest-path layer assignment ----
        // Sort component in topological order for correct layer computation
        QList<QString> compTopo;
        for (const QString &uid : topoOrder) {
            if (compSet.contains(uid))
                compTopo.append(uid);
        }
        // Add any nodes not in topoOrder (e.g. from cycles fallback)
        for (const QString &uid : component) {
            if (!topoIndex.contains(uid))
                compTopo.append(uid);
        }

        QHash<QString, int> layer;
        int maxLayer = 0;

        for (const QString &uid : compTopo) {
            int myLayer = 0;
            for (const QString &pred : predecessors.value(uid)) {
                if (layer.contains(pred))
                    myLayer = qMax(myLayer, layer[pred] + 1);
            }
            layer[uid] = myLayer;
            maxLayer = qMax(maxLayer, myLayer);
        }

        // ---- Phase 2b: Vertex promotion ----
        {
            bool changed = true;
            int iterations = 0;
            while (changed && iterations < 10) {
                changed = false;
                ++iterations;
                for (int i = compTopo.size() - 1; i >= 0; --i) {
                    const QString &uid = compTopo[i];
                    const QList<QString> &succs = successors.value(uid);
                    if (succs.isEmpty())
                        continue;

                    int minSuccLayer = INT_MAX;
                    bool allInComp = true;
                    for (const QString &s : succs) {
                        if (!layer.contains(s)) {
                            allInComp = false;
                            break;
                        }
                        minSuccLayer = qMin(minSuccLayer, layer[s]);
                    }
                    if (!allInComp || minSuccLayer == INT_MAX)
                        continue;

                    int targetLayer = minSuccLayer - 1;
                    if (targetLayer > layer[uid]) {
                        layer[uid] = targetLayer;
                        changed = true;
                    }
                }
            }

            // Recompute maxLayer after promotion
            maxLayer = 0;
            for (const QString &uid : compTopo)
                maxLayer = qMax(maxLayer, layer[uid]);
        }

        // Group real nodes by layer
        QHash<int, QList<QString>> layerNodes;
        for (const QString &uid : compTopo)
            layerNodes[layer[uid]].append(uid);

        // ---- Phase 3: Dummy vertex insertion ----
        struct DummyInfo { QString sourceUid; QString targetUid; int seqIndex; };
        QHash<QString, DummyInfo> dummyEdgeMap;

        QHash<QString, QList<QString>> compPred;
        QHash<QString, QList<QString>> compSucc;
        QSet<QString> dummyNodes;
        int dummyCounter = 0;

        for (const auto &edge : edges) {
            if (!compSet.contains(edge.first) || !compSet.contains(edge.second))
                continue;
            int fromLayer = layer[edge.first];
            int toLayer = layer[edge.second];
            int span = toLayer - fromLayer;

            if (span <= 1) {
                compPred[edge.second].append(edge.first);
                compSucc[edge.first].append(edge.second);
            } else {
                // Long edge: insert dummies and record their origin
                QString prev = edge.first;
                int seq = 0;
                for (int l = fromLayer + 1; l < toLayer; ++l) {
                    QString dummyId = QStringLiteral("__dummy_%1_%2").arg(dummyCounter++).arg(l);
                    dummyNodes.insert(dummyId);
                    layer[dummyId] = l;
                    layerNodes[l].append(dummyId);

                    DummyInfo di;
                    di.sourceUid = edge.first;
                    di.targetUid = edge.second;
                    di.seqIndex = seq++;
                    dummyEdgeMap[dummyId] = di;

                    compPred[dummyId].append(prev);
                    compSucc[prev].append(dummyId);
                    prev = dummyId;
                }
                compPred[edge.second].append(prev);
                compSucc[prev].append(edge.second);
            }
        }

        // ---- Phase 3b-pre: Identify spine nodes per layer for center seeding ----
        QHash<int, QString> spineNodeInLayer;
        QSet<QString> spineNodes;
        {
            for (const QString &uid : compTopo) {
                if (!cpSet.contains(uid))
                    continue;
                int l = layer[uid];
                if (!spineNodeInLayer.contains(l))
                    spineNodeInLayer[l] = uid;
            }

            // For layers with no real spine node, pick a spine-edge dummy
            for (auto it = dummyEdgeMap.begin(); it != dummyEdgeMap.end(); ++it) {
                const QString &dummyId = it.key();
                const DummyInfo &di = it.value();
                int l = layer[dummyId];
                if (!spineNodeInLayer.contains(l)
                    && cpSet.contains(di.sourceUid)
                    && cpSet.contains(di.targetUid)) {
                    spineNodeInLayer[l] = dummyId;
                }
            }

            for (auto it = spineNodeInLayer.begin(); it != spineNodeInLayer.end(); ++it)
                spineNodes.insert(it.value());
        }

        // ---- Phase 3b: Seed initial ordering with spine centered ----
        for (int l = 0; l <= maxLayer; ++l) {
            QList<QString> &nodes = layerNodes[l];
            if (nodes.size() <= 1)
                continue;

            int spIdx = -1;
            for (int i = 0; i < nodes.size(); ++i) {
                if (spineNodes.contains(nodes[i])) {
                    spIdx = i;
                    break;
                }
            }
            if (spIdx < 0)
                continue;

            int centerPos = nodes.size() / 2;
            if (spIdx != centerPos) {
                QString spNode = nodes[spIdx];
                nodes.removeAt(spIdx);
                nodes.insert(centerPos, spNode);
            }
        }

        // ---- Phase 4: Adaptive median crossing minimization ----
        auto medianOfNeighbors = [](const QList<QString> &neighbors,
                                    const QHash<QString, int> &posMap) -> qreal {
            QList<int> positions;
            for (const QString &n : neighbors) {
                if (posMap.contains(n))
                    positions.append(posMap[n]);
            }
            if (positions.isEmpty())
                return -1.0;
            std::sort(positions.begin(), positions.end());
            int sz = positions.size();
            if (sz % 2 == 1)
                return positions[sz / 2];
            return (positions[sz / 2 - 1] + positions[sz / 2]) / 2.0;
        };

        QHash<QString, int> nodePosition = buildPositionMap(maxLayer, layerNodes);
        int bestCrossings = countCrossings(maxLayer, layerNodes, nodePosition, compSucc);
        auto bestOrdering = snapshotOrdering(maxLayer, layerNodes);

        constexpr int maxSweeps = 24;
        constexpr int earlyStopAfter = 3;
        int noImprovementCount = 0;

        for (int sweep = 0; sweep < maxSweeps; ++sweep) {
            if (sweep % 2 == 0) {
                // Forward sweep: order each layer by median of predecessors
                for (int l = 1; l <= maxLayer; ++l) {
                    QList<QString> &nodes = layerNodes[l];
                    if (nodes.size() <= 1)
                        continue;

                    QHash<QString, qreal> med;
                    for (const QString &uid : nodes) {
                        qreal m = medianOfNeighbors(compPred.value(uid), nodePosition);
                        med[uid] = (m >= 0) ? m : nodePosition.value(uid, 0);
                    }

                    std::stable_sort(nodes.begin(), nodes.end(),
                        [&](const QString &a, const QString &b) {
                            return med[a] < med[b];
                        });
                }
            } else {
                // Backward sweep: order each layer by median of successors
                for (int l = maxLayer - 1; l >= 0; --l) {
                    QList<QString> &nodes = layerNodes[l];
                    if (nodes.size() <= 1)
                        continue;

                    QHash<QString, qreal> med;
                    for (const QString &uid : nodes) {
                        qreal m = medianOfNeighbors(compSucc.value(uid), nodePosition);
                        med[uid] = (m >= 0) ? m : nodePosition.value(uid, 0);
                    }

                    std::stable_sort(nodes.begin(), nodes.end(),
                        [&](const QString &a, const QString &b) {
                            return med[a] < med[b];
                        });
                }
            }

            nodePosition = buildPositionMap(maxLayer, layerNodes);
            int crossings = countCrossings(maxLayer, layerNodes, nodePosition, compSucc);

            if (crossings < bestCrossings) {
                bestCrossings = crossings;
                bestOrdering = snapshotOrdering(maxLayer, layerNodes);
                noImprovementCount = 0;
            } else {
                ++noImprovementCount;
                if (noImprovementCount >= earlyStopAfter)
                    break;
            }
        }

        // Restore best ordering
        layerNodes = bestOrdering;

        // ---- Phase 5: Brandes-Kopf compact coordinate assignment ----
        nodePosition = buildPositionMap(maxLayer, layerNodes);
        qreal minSep = withinExtent + withinGap;
        QHash<QString, qreal> compactY = brandesKopfYCoordinates(
            maxLayer, layerNodes, layer, nodePosition,
            compPred, compSucc, minSep);

        QHash<QString, QPointF> allPositions;
        for (int l = 0; l <= maxLayer; ++l) {
            qreal u = l * (layerExtent + layerGap);
            for (const QString &uid : layerNodes[l])
                allPositions[uid] = QPointF(u, compactY.value(uid, 0) + componentYOffset);
        }

        // ---- Phase 5b: Spine center-alignment ----
        if (m_spineAlignment) {
            QList<qreal> cpYValues;
            for (auto it = spineNodeInLayer.begin(); it != spineNodeInLayer.end(); ++it) {
                const QString &uid = it.value();
                if (allPositions.contains(uid))
                    cpYValues.append(allPositions[uid].y() + withinExtent / 2.0);
            }

            if (!cpYValues.isEmpty()) {
                QList<qreal> sorted = cpYValues;
                std::sort(sorted.begin(), sorted.end());
                qreal targetY = sorted[sorted.size() / 2];

                for (auto it = spineNodeInLayer.begin(); it != spineNodeInLayer.end(); ++it) {
                    int l = it.key();
                    const QString &spUid = it.value();
                    if (!allPositions.contains(spUid))
                        continue;
                    qreal spCenter = allPositions[spUid].y() + withinExtent / 2.0;
                    qreal delta = targetY - spCenter;

                    if (qAbs(delta) < 0.5)
                        continue;

                    for (const QString &uid : layerNodes[l]) {
                        QPointF p = allPositions[uid];
                        allPositions[uid] = QPointF(p.x(), p.y() + delta);
                    }
                }
            }
        }

        // ---- Phase 5b-post: Normalize to non-negative within-layer coord ----
        {
            qreal minY = std::numeric_limits<qreal>::max();
            for (auto it = allPositions.begin(); it != allPositions.end(); ++it)
                minY = qMin(minY, it.value().y());
            if (minY < componentYOffset) {
                qreal shift = componentYOffset - minY;
                for (auto it = allPositions.begin(); it != allPositions.end(); ++it)
                    it.value().setY(it.value().y() + shift);
            }
        }

        // Compute maxHeight for component stacking after normalization
        qreal maxHeight = 0;
        {
            qreal maxY = 0;
            for (auto it = allPositions.begin(); it != allPositions.end(); ++it)
                maxY = qMax(maxY, it.value().y() + withinExtent);
            maxHeight = maxY - componentYOffset;
        }

        // ---- Phase 5c: Extract real node positions and dummy waypoints ----
        for (auto it = allPositions.begin(); it != allPositions.end(); ++it) {
            const QString &uid = it.key();
            const QPointF &pos = it.value();

            if (dummyNodes.contains(uid)) {
                const DummyInfo &di = dummyEdgeMap[uid];
                auto key = qMakePair(di.sourceUid, di.targetUid);
                QPointF wp(pos.x() + layerExtent / 2.0, pos.y() + withinExtent / 2.0);

                QList<QPointF> &wps = result.edgeWaypoints[key];
                while (wps.size() <= di.seqIndex)
                    wps.append(QPointF());
                wps[di.seqIndex] = wp;
            } else {
                result.nodePositions[uid] = pos;
            }
        }

        // Advance within-axis offset for next component
        componentYOffset += maxHeight + 2 * withinGap;
    }

    // ---- Phase 6: (u, v) -> (x, y) transform per direction ----
    // Internal frame uses u on X and v on Y. We mirror the layer axis
    // for RL/BT, then swap axes for TB/BT.
    if (mirror) {
        qreal maxU = 0;
        for (auto it = result.nodePositions.begin(); it != result.nodePositions.end(); ++it)
            maxU = qMax(maxU, it.value().x());
        // Node positions are rect top-lefts: newU_topleft = maxU - u.
        // Waypoints are single points: newU_point = (maxU + layerExtent) - u.
        for (auto it = result.nodePositions.begin(); it != result.nodePositions.end(); ++it)
            it.value().setX(maxU - it.value().x());
        for (auto it = result.edgeWaypoints.begin(); it != result.edgeWaypoints.end(); ++it) {
            QList<QPointF> &wps = it.value();
            for (int i = 0; i < wps.size(); ++i)
                wps[i].setX(maxU + layerExtent - wps[i].x());
        }
    }
    if (swapAxes) {
        for (auto it = result.nodePositions.begin(); it != result.nodePositions.end(); ++it) {
            QPointF p = it.value();
            it.value() = QPointF(p.y(), p.x());
        }
        for (auto it = result.edgeWaypoints.begin(); it != result.edgeWaypoints.end(); ++it) {
            QList<QPointF> &wps = it.value();
            for (int i = 0; i < wps.size(); ++i)
                wps[i] = QPointF(wps[i].y(), wps[i].x());
        }
    }

    return result;
}

} // namespace Graffodil
