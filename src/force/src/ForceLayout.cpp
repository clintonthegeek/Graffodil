#include <graffodil/ForceLayout.h>
#include <graffodil/MultilevelLayout.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include <graffodil/QuadTree.h>

#include <QQueue>
#include <QRandomGenerator>
#include <QSet>
#include <cmath>

namespace Graffodil {

static constexpr double MIN_CANVAS_AREA = 10000.0;

// ---------------------------------------------------------------------------
// LayoutEngine interface
// ---------------------------------------------------------------------------

void ForceLayout::setNodes(const QList<IGraphNode *> &nodes)
{
    m_inputNodes = nodes;
    m_initialized = false;
}

void ForceLayout::setEdges(const QList<IGraphEdge *> &edges)
{
    m_inputEdges = edges;
    m_initialized = false;
}

void ForceLayout::setCenterForce(qreal f) { m_centerForce = f; }
void ForceLayout::setRepelForce(qreal f) { m_repelForce = f; }
void ForceLayout::setLinkForce(qreal f) { m_linkForce = f; }
void ForceLayout::setLinkDistance(qreal d) { m_linkDistance = d; }

bool ForceLayout::isConverged() const { return m_converged; }

LayoutResult ForceLayout::currentPositions() const
{
    LayoutResult result;
    const int n = m_positions.size();
    result.nodePositions.reserve(n);
    for (int i = 0; i < n; ++i)
        result.nodePositions[m_ids[i]] = m_positions[i];
    return result;
}

void ForceLayout::pinNode(const QString &id, const QPointF &pos)
{
    // If not yet initialized, store as pending — will be applied in initialize()
    if (!m_initialized) {
        m_pendingPins[id] = pos;
        return;
    }

    auto it = m_idToIndex.find(id);
    if (it == m_idToIndex.end())
        return;
    int idx = it.value();
    m_pinned[idx] = true;
    m_positions[idx] = pos;
}

void ForceLayout::unpinNode(const QString &id)
{
    if (!m_initialized) {
        m_pendingPins.remove(id);
        return;
    }

    auto it = m_idToIndex.find(id);
    if (it == m_idToIndex.end())
        return;
    m_pinned[it.value()] = false;
}

// ---------------------------------------------------------------------------
// buildInternalStructures — convert IGraphNode/IGraphEdge to integer-indexed arrays
// ---------------------------------------------------------------------------

void ForceLayout::buildInternalStructures()
{
    const int n = m_inputNodes.size();

    m_ids.resize(n);
    m_positions.resize(n);
    m_pinned.resize(n);
    m_idToIndex.clear();
    m_idToIndex.reserve(n);
    m_degree.clear();

    for (int i = 0; i < n; ++i) {
        const QString id = m_inputNodes[i]->nodeId();
        m_ids[i] = id;
        m_positions[i] = QPointF(0, 0); // will be placed by BFS or multilevel
        m_pinned[i] = false;
        m_idToIndex[id] = i;
    }

    m_edges.clear();
    m_edges.reserve(m_inputEdges.size());

    for (const auto *edge : m_inputEdges) {
        const QString srcId = edge->sourceNode()->nodeId();
        const QString tgtId = edge->targetNode()->nodeId();
        auto srcIt = m_idToIndex.find(srcId);
        auto tgtIt = m_idToIndex.find(tgtId);
        if (srcIt == m_idToIndex.end() || tgtIt == m_idToIndex.end())
            continue;
        int s = srcIt.value();
        int t = tgtIt.value();
        if (s == t)
            continue;
        m_edges.append({s, t});
        m_degree[srcId]++;
        m_degree[tgtId]++;
    }
}

// ---------------------------------------------------------------------------
// initialize — build internal structures + BFS initial placement
// ---------------------------------------------------------------------------

void ForceLayout::initialize()
{
    if (m_initialized)
        return;

    buildInternalStructures();

    const int n = m_positions.size();
    if (n <= 1) {
        m_initialized = true;
        m_converged = true;
        return;
    }

    // BFS initial placement (concentric rings per connected component)
    // Build adjacency
    QVector<QVector<int>> adjacency(n);
    for (const auto &edge : m_edges) {
        adjacency[edge.src].append(edge.tgt);
        adjacency[edge.tgt].append(edge.src);
    }

    // Find connected components via BFS
    QVector<QVector<int>> components;
    QVector<bool> visited(n, false);

    for (int i = 0; i < n; ++i) {
        if (visited[i]) continue;

        QVector<int> component;
        QQueue<int> queue;
        queue.enqueue(i);
        visited[i] = true;

        while (!queue.isEmpty()) {
            int current = queue.dequeue();
            component.append(current);

            for (int neighbor : adjacency[current]) {
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    queue.enqueue(neighbor);
                }
            }
        }
        components.append(component);
    }

    // Sort components by size descending
    std::sort(components.begin(), components.end(),
              [](const QVector<int> &a, const QVector<int> &b) {
                  return a.size() > b.size();
              });

    double offsetX = 0.0;
    double gap = m_linkDistance * 0.5;

    for (const auto &component : components) {
        if (component.size() == 1) {
            if (!m_pinned[component[0]])
                m_positions[component[0]] = QPointF(offsetX, 0.0);
            offsetX += gap;
            continue;
        }

        QSet<int> componentSet(component.begin(), component.end());

        // Two-pass BFS to find approximate diameter endpoints
        auto bfsFarthest = [&](int startIdx) -> int {
            QHash<int, int> dist;
            QQueue<int> q;
            q.enqueue(startIdx);
            dist[startIdx] = 0;
            int farthest = startIdx;
            int maxDist = 0;

            while (!q.isEmpty()) {
                int current = q.dequeue();
                for (int neighbor : adjacency[current]) {
                    if (!dist.contains(neighbor) && componentSet.contains(neighbor)) {
                        dist[neighbor] = dist[current] + 1;
                        if (dist[neighbor] > maxDist) {
                            maxDist = dist[neighbor];
                            farthest = neighbor;
                        }
                        q.enqueue(neighbor);
                    }
                }
            }
            return farthest;
        };

        int endpoint1 = bfsFarthest(component[0]);

        // BFS from endpoint1 to assign layers
        QHash<int, int> layer;
        QQueue<int> queue;
        queue.enqueue(endpoint1);
        layer[endpoint1] = 0;
        int maxLayer = 0;

        while (!queue.isEmpty()) {
            int current = queue.dequeue();
            for (int neighbor : adjacency[current]) {
                if (!layer.contains(neighbor) && componentSet.contains(neighbor)) {
                    layer[neighbor] = layer[current] + 1;
                    maxLayer = std::max(maxLayer, layer[neighbor]);
                    queue.enqueue(neighbor);
                }
            }
        }

        // Count nodes per layer for angular distribution
        QHash<int, int> layerCounts;
        QHash<int, int> layerCurrentIndex;
        for (int nodeIdx : component) {
            int l = layer.value(nodeIdx, 0);
            layerCounts[l]++;
            layerCurrentIndex[l] = 0;
        }

        // Place nodes on concentric rings
        auto *rng = QRandomGenerator::global();
        double componentMaxRadius = 0.0;

        for (int nodeIdx : component) {
            if (m_pinned[nodeIdx]) continue;

            int l = layer.value(nodeIdx, 0);
            double radius = l * m_linkDistance;
            componentMaxRadius = std::max(componentMaxRadius, radius);

            int count = layerCounts[l];
            int index = layerCurrentIndex[l]++;

            double angle;
            if (count == 1) {
                angle = 0.0;
            } else {
                double angleStep = 2.0 * M_PI / count;
                angle = index * angleStep;
            }
            // Small jitter to prevent exact overlaps
            angle += (rng->generateDouble() - 0.5) * 0.2;

            m_positions[nodeIdx] = QPointF(
                offsetX + radius * std::cos(angle),
                radius * std::sin(angle)
            );
        }

        offsetX += 2.0 * componentMaxRadius + gap;
    }

    // Apply any pending pin requests
    for (auto it = m_pendingPins.constBegin(); it != m_pendingPins.constEnd(); ++it) {
        auto idxIt = m_idToIndex.find(it.key());
        if (idxIt != m_idToIndex.end()) {
            int idx = idxIt.value();
            m_pinned[idx] = true;
            m_positions[idx] = it.value();
        }
    }
    m_pendingPins.clear();

    // Initialize simulation state
    m_displacements.resize(n);
    m_previousForces.resize(n);
    for (int i = 0; i < n; ++i) {
        m_displacements[i] = QPointF(0, 0);
        m_previousForces[i] = QPointF(0, 0);
    }
    m_globalSpeed = 1.0;
    m_energy = 0.0;
    m_prevEnergy = 0.0;
    m_iteration = 0;
    m_stableCount = 0;
    m_energyStableCount = 0;
    m_converged = false;
    m_initialized = true;
}

// ---------------------------------------------------------------------------
// estimateCanvasArea
// ---------------------------------------------------------------------------

double ForceLayout::estimateCanvasArea() const
{
    const int n = m_positions.size();
    if (n < 2)
        return MIN_CANVAS_AREA;

    double minX = m_positions[0].x(), maxX = minX;
    double minY = m_positions[0].y(), maxY = minY;

    for (int i = 1; i < n; ++i) {
        minX = std::min(minX, m_positions[i].x());
        maxX = std::max(maxX, m_positions[i].x());
        minY = std::min(minY, m_positions[i].y());
        maxY = std::max(maxY, m_positions[i].y());
    }

    double width = std::max(maxX - minX, 100.0);
    double height = std::max(maxY - minY, 100.0);

    return std::max(width * height * 4.0, MIN_CANVAS_AREA);
}

// ---------------------------------------------------------------------------
// tick — one simulation step, returns false when converged
// ---------------------------------------------------------------------------

bool ForceLayout::tick()
{
    if (!m_initialized)
        initialize();

    if (m_converged)
        return false;

    const int n = m_positions.size();
    if (n == 0) {
        m_converged = true;
        return false;
    }

    // 1. Compute canvas area
    double canvasArea = estimateCanvasArea();
    double k = m_linkDistance;

    // 2. Reset current displacements
    for (int i = 0; i < n; ++i)
        m_displacements[i] = QPointF(0, 0);

    // 3. Compute repulsive forces
    if (n > BARNES_HUT_THRESHOLD) {
        // Barnes-Hut via QuadTree
        QVector<qreal> masses(n);
        for (int i = 0; i < n; ++i)
            masses[i] = m_degree.value(m_ids[i], 0) + 1.0;

        QuadTree quadTree;
        quadTree.build(m_positions, masses);

        for (int i = 0; i < n; ++i) {
            double nodeMass = m_degree.value(m_ids[i], 0) + 1.0;
            QPointF repulsion = quadTree.computeRepulsion(
                m_positions[i], m_repelForce, nodeMass, 0.8);
            m_displacements[i] += repulsion;
        }
    } else {
        // Naive O(n^2) repulsion with degree weighting
        for (int i = 0; i < n; ++i) {
            double degI = m_degree.value(m_ids[i], 0) + 1.0;
            for (int j = i + 1; j < n; ++j) {
                QPointF delta = m_positions[i] - m_positions[j];
                double dist = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
                dist = std::max(dist, EPSILON);

                double degJ = m_degree.value(m_ids[j], 0) + 1.0;
                double force = m_repelForce * std::sqrt(degI * degJ) / (dist * dist);

                QPointF normalized(delta.x() / dist, delta.y() / dist);
                m_displacements[i] += normalized * force;
                m_displacements[j] -= normalized * force;
            }
        }
    }

    // 4. Compute attractive forces along edges
    for (const auto &edge : m_edges) {
        QPointF delta = m_positions[edge.src] - m_positions[edge.tgt];
        double dist = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
        dist = std::max(dist, EPSILON);

        double force = (dist - k) * m_linkForce;

        QPointF normalized(delta.x() / dist, delta.y() / dist);
        m_displacements[edge.src] -= normalized * force;
        m_displacements[edge.tgt] += normalized * force;
    }

    // 5. Center force (pull toward origin), degree-weighted
    for (int i = 0; i < n; ++i) {
        double deg = m_degree.value(m_ids[i], 0) + 1.0;
        m_displacements[i] -= m_positions[i] * m_centerForce * deg;
    }

    // 6. ForceAtlas2 adaptive speed
    double globalSwinging = 0.0;
    double globalTraction = 0.0;

    for (int i = 0; i < n; ++i) {
        double deg = m_degree.value(m_ids[i], 0) + 1.0;

        double swingX = m_displacements[i].x() - m_previousForces[i].x();
        double swingY = m_displacements[i].y() - m_previousForces[i].y();
        double swinging = std::sqrt(swingX * swingX + swingY * swingY);

        double tractX = (m_displacements[i].x() + m_previousForces[i].x()) / 2.0;
        double tractY = (m_displacements[i].y() + m_previousForces[i].y()) / 2.0;
        double traction = std::sqrt(tractX * tractX + tractY * tractY);

        globalSwinging += deg * swinging;
        globalTraction += deg * traction;
    }

    double tolerance = 1.0;
    if (globalSwinging > EPSILON)
        m_globalSpeed = tolerance * globalTraction / globalSwinging;

    m_globalSpeed = std::min(m_globalSpeed, m_globalSpeed * 1.5);
    m_globalSpeed = std::max(m_globalSpeed, EPSILON);

    // 7. Apply displacement with per-node speed
    double maxDisplacement = 0.0;
    m_energy = 0.0;

    for (int i = 0; i < n; ++i) {
        if (m_pinned[i]) continue;

        double deg = m_degree.value(m_ids[i], 0) + 1.0;

        // Per-node swinging
        double swingX = m_displacements[i].x() - m_previousForces[i].x();
        double swingY = m_displacements[i].y() - m_previousForces[i].y();
        double swinging = std::sqrt(swingX * swingX + swingY * swingY);

        // Per-node speed
        double localSpeed = m_globalSpeed / (1.0 + m_globalSpeed * std::sqrt(swinging));

        // Compute displacement
        double dx = m_displacements[i].x() * localSpeed;
        double dy = m_displacements[i].y() * localSpeed;
        double mag = std::sqrt(dx * dx + dy * dy);

        // Cap displacement at 10x a reasonable node radius (10.0 default)
        double maxDisp = 100.0;
        if (mag > maxDisp) {
            double scale = maxDisp / mag;
            dx *= scale;
            dy *= scale;
            mag = maxDisp;
        }

        m_positions[i] += QPointF(dx, dy);
        maxDisplacement = std::max(maxDisplacement, mag);
        m_energy += deg * std::sqrt(m_displacements[i].x() * m_displacements[i].x() +
                                     m_displacements[i].y() * m_displacements[i].y());
    }

    // Save forces for next iteration's swinging calculation
    m_previousForces = m_displacements;

    // 8. Convergence detection

    // Energy-plateau convergence: converged when energy stops changing
    // run-over-run. (A correctly cooling simulation *decreases* energy every
    // tick — decreasing energy means still improving, not converged.)
    if (m_prevEnergy > EPSILON) {
        const double relChange =
            std::abs(m_energy - m_prevEnergy) / m_prevEnergy;
        if (relChange < 0.001) {
            ++m_energyStableCount;
            if (m_energyStableCount >= 10)
                m_converged = true;
        } else {
            m_energyStableCount = 0;
        }
    }

    // Displacement-based fallback
    // Use a threshold relative to link distance so it is independent of
    // canvas scale (the cap on node movement is 100 px — a canvas-relative
    // threshold would fire on the very first tick for large graphs).
    double convergenceThreshold = m_linkDistance * 0.1;
    if (maxDisplacement < convergenceThreshold) {
        ++m_stableCount;
        if (m_stableCount >= STABLE_ITERATIONS_REQUIRED) {
            m_converged = true;
        }
    } else {
        m_stableCount = 0;
    }

    m_prevEnergy = m_energy;
    ++m_iteration;

    return !m_converged;
}

// ---------------------------------------------------------------------------
// compute — full run to convergence (multilevel + ticks)
// ---------------------------------------------------------------------------

LayoutResult ForceLayout::compute()
{
    if (!m_initialized)
        initialize();

    const int n = m_positions.size();
    if (n <= 1)
        return currentPositions();

    // Use multilevel optimization for initial positions if graph is large enough
    {
        QVector<int> edgeSrc, edgeTgt;
        edgeSrc.reserve(m_edges.size());
        edgeTgt.reserve(m_edges.size());
        for (const auto &e : m_edges) {
            edgeSrc.append(e.src);
            edgeTgt.append(e.tgt);
        }

        MultilevelConfig config;
        config.repelForce = m_repelForce;
        config.linkForce = m_linkForce;
        config.linkDistance = m_linkDistance;
        config.centerForce = m_centerForce;

        QVector<QPointF> optimized = MultilevelLayout::computeLayout(
            m_positions, edgeSrc, edgeTgt, config);

        // Apply multilevel positions (but not for pinned nodes)
        for (int i = 0; i < n; ++i) {
            if (!m_pinned[i])
                m_positions[i] = optimized[i];
        }
    }

    // Run ticks until convergence
    int maxIterations = 5000;
    for (int i = 0; i < maxIterations; ++i) {
        if (!tick())
            break;
    }

    return currentPositions();
}

} // namespace Graffodil
