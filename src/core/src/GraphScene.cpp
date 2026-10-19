#include <graffodil/GraphScene.h>
#include <graffodil/GraphTool.h>
#include <graffodil/GroupItem.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QKeyEvent>
#include <QUuid>

namespace Graffodil {

namespace {
static constexpr int kTransientSceneItemRole = 0x47524631; // "GRF1"
}

void GraphScene::addNode(IGraphNode *node)
{
    if (!node || m_nodes.contains(node->nodeId()))
        return;
    m_nodes.insert(node->nodeId(), node);
    // Only assign default z if consumer hasn't set one.
    QGraphicsItem *gi = node->graphicsItem();
    if (gi->zValue() == 0.0)
        gi->setZValue(m_edgesAboveNodes ? DefaultEdgeZ : DefaultNodeZ);
    addItem(gi);
    emit nodeAdded(node);
}

void GraphScene::removeNode(const QString &id)
{
    auto it = m_nodes.find(id);
    if (it == m_nodes.end())
        return;
    IGraphNode *node = it.value();

    // Cascade: remove any edges that reference this node first.
    // Otherwise those edges would hold dangling pointers to the removed
    // node, and any subsequent access (including paint-time bounding-rect
    // queries) would crash.
    QList<QString> connectedIds;
    const QList<IGraphEdge *> touching = m_edgesByNode.values(id);
    connectedIds.reserve(touching.size());
    for (auto *edge : touching)
        connectedIds.append(edge->edgeId());
    for (const auto &eid : connectedIds)
        removeEdge(eid);

    removeItem(node->graphicsItem());
    m_nodes.erase(it);

    // Cascade into group membership: drop this node from every group.
    // The group item itself stays; the region just shrinks.
    bool anyGroupTouched = false;
    for (auto git = m_groupMembers.begin(); git != m_groupMembers.end(); ++git) {
        if (git.value().remove(id))
            anyGroupTouched = true;
    }
    if (anyGroupTouched)
        refreshGroups();

    emit nodeRemoved(id);
}

QList<IGraphEdge *> GraphScene::connectedEdges(const QString &nodeId) const
{
    return m_edgesByNode.values(nodeId);
}

IGraphNode *GraphScene::nodeForId(const QString &id) const
{
    return m_nodes.value(id, nullptr);
}

QList<IGraphNode *> GraphScene::nodes() const
{
    return m_nodes.values();
}

void GraphScene::addEdge(IGraphEdge *edge)
{
    if (!edge || m_edges.contains(edge->edgeId()))
        return;
    m_edges.insert(edge->edgeId(), edge);
    m_edgesByNode.insert(edge->sourceNodeId(), edge);
    if (edge->targetNodeId() != edge->sourceNodeId())
        m_edgesByNode.insert(edge->targetNodeId(), edge);
    QGraphicsItem *gi = edge->graphicsItem();
    if (gi->zValue() == 0.0)
        gi->setZValue(m_edgesAboveNodes ? DefaultNodeZ : DefaultEdgeZ);
    addItem(gi);
    emit edgeAdded(edge);
}

void GraphScene::setEdgesAboveNodes(bool above)
{
    if (m_edgesAboveNodes == above)
        return;
    m_edgesAboveNodes = above;
    // Re-assign z on items still at either default value.
    for (auto *node : m_nodes) {
        QGraphicsItem *gi = node->graphicsItem();
        qreal z = gi->zValue();
        if (z == DefaultNodeZ || z == DefaultEdgeZ)
            gi->setZValue(above ? DefaultEdgeZ : DefaultNodeZ);
    }
    for (auto *edge : m_edges) {
        QGraphicsItem *gi = edge->graphicsItem();
        qreal z = gi->zValue();
        if (z == DefaultNodeZ || z == DefaultEdgeZ)
            gi->setZValue(above ? DefaultNodeZ : DefaultEdgeZ);
    }
}

bool GraphScene::edgesAboveNodes() const
{
    return m_edgesAboveNodes;
}

void GraphScene::removeEdge(const QString &id)
{
    auto it = m_edges.find(id);
    if (it == m_edges.end())
        return;
    IGraphEdge *edge = it.value();
    // Remove from adjacency index before erasing — covers both current
    // endpoints regardless of any prior reverse() call.
    m_edgesByNode.remove(edge->sourceNodeId(), edge);
    m_edgesByNode.remove(edge->targetNodeId(), edge);
    // The edge object may outlive the scene registry (a consumer holds it), and
    // removeNode() may free the node next: drop the endpoint pointers first so
    // a later adjust() cannot dereference a dead node.
    edge->detachEndpoints();
    removeItem(edge->graphicsItem());
    m_edges.erase(it);
    emit edgeRemoved(id);
}

IGraphEdge *GraphScene::edgeForId(const QString &id) const
{
    return m_edges.value(id, nullptr);
}

IGraphEdge *GraphScene::edgeBetween(const QString &sourceId, const QString &targetId) const
{
    for (auto it = m_edgesByNode.constFind(sourceId);
         it != m_edgesByNode.constEnd() && it.key() == sourceId; ++it) {
        IGraphEdge *edge = it.value();
        if (edge->sourceNodeId() == sourceId && edge->targetNodeId() == targetId)
            return edge;
    }
    return nullptr;
}

IGraphEdge *GraphScene::edgeBetweenUndirected(const QString &a, const QString &b) const
{
    for (auto it = m_edgesByNode.constFind(a);
         it != m_edgesByNode.constEnd() && it.key() == a; ++it) {
        IGraphEdge *edge = it.value();
        const QString s = edge->sourceNodeId();
        const QString t = edge->targetNodeId();
        if ((s == a && t == b) || (s == b && t == a))
            return edge;
    }
    return nullptr;
}

QList<IGraphEdge *> GraphScene::edges() const
{
    return m_edges.values();
}

void GraphScene::fitSceneRectToContent(qreal margin)
{
    QRectF items = itemsBoundingRect();
    if (items.isEmpty()) {
        setSceneRect(-margin, -margin, 2 * margin, 2 * margin);
        return;
    }
    setSceneRect(items.adjusted(-margin, -margin, margin, margin));
}

GraphScene::ClearResult GraphScene::clearGraph()
{
    ClearResult result;
    // Snapshot values first so we can iterate safely.
    result.edges = m_edges.values();
    result.nodes = m_nodes.values();

    // Remove from the registries before emitting: a slot that re-enters the
    // scene during a removal must not see half-cleared state.
    for (auto *edge : result.edges) {
        removeItem(edge->graphicsItem());
        edge->detachEndpoints();
    }
    m_edges.clear();
    m_edgesByNode.clear();
    for (auto *edge : result.edges)
        emit edgeRemoved(edge->edgeId());

    for (auto *node : result.nodes)
        removeItem(node->graphicsItem());
    m_nodes.clear();
    for (auto *node : result.nodes)
        emit nodeRemoved(node->nodeId());

    // Groups are scene-owned decorations; clearGraph() removes them too.
    for (auto *g : m_groups) {
        removeItem(g);
        delete g;
    }
    m_groups.clear();
    m_groupMembers.clear();

    if (m_groupsChangeConnected) {
        disconnect(this, &QGraphicsScene::changed,
                   this, &GraphScene::refreshGroupsFromSceneChange);
        m_groupsChangeConnected = false;
    }

    return result;
}

void GraphScene::setActiveTool(GraphTool *tool)
{
    if (m_activeTool)
        m_activeTool->deactivate();
    m_activeTool = tool;
    if (m_activeTool)
        m_activeTool->activate(this);
}

GraphTool *GraphScene::activeTool() const
{
    return m_activeTool;
}

QList<IGraphNode *> GraphScene::selectedNodes() const
{
    QList<IGraphNode *> result;
    for (auto *node : m_nodes) {
        if (node->graphicsItem()->isSelected())
            result.append(node);
    }
    return result;
}

QList<IGraphEdge *> GraphScene::selectedEdges() const
{
    QList<IGraphEdge *> result;
    for (auto *edge : m_edges) {
        if (edge->graphicsItem()->isSelected())
            result.append(edge);
    }
    return result;
}

QString GraphScene::selectedNodeId() const
{
    for (auto *node : m_nodes) {
        if (node->graphicsItem()->isSelected())
            return node->nodeId();
    }
    return {};
}

void GraphScene::selectNode(const QString &id)
{
    IGraphNode *node = m_nodes.value(id, nullptr);
    if (!node)
        return;
    clearSelection();
    node->graphicsItem()->setSelected(true);
}

// ---------------------------------------------------------------------
// Group regions
// ---------------------------------------------------------------------
//
// Tracking strategy: we hook QGraphicsScene::changed (the signal Qt
// emits after any item invalidation, including position changes from
// interactive drags and programmatic setPos) and refresh all registered
// groups. This is cheap for a small number of groups and correctly
// covers every case that moves a node — we don't have to instrument
// each tool or layout separately.

QString GraphScene::addGroup(const QSet<QString> &memberNodeIds,
                             const GroupStyle &style)
{
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);

    auto *item = new GroupItem(id, style);
    m_groups.insert(id, item);
    m_groupMembers.insert(id, memberNodeIds);
    addItem(item);

    if (!m_groupsChangeConnected) {
        connect(this, &QGraphicsScene::changed,
                this, &GraphScene::refreshGroupsFromSceneChange,
                Qt::UniqueConnection);
        m_groupsChangeConnected = true;
    }

    refreshGroups();
    return id;
}

void GraphScene::setGroupStyle(const QString &groupId, const GroupStyle &style)
{
    GroupItem *item = m_groups.value(groupId, nullptr);
    if (!item)
        return;
    item->setStyle(style);
    // Padding change affects bounds; recompute.
    refreshGroups();
}

void GraphScene::setGroupMembers(const QString &groupId,
                                 const QSet<QString> &memberNodeIds)
{
    if (!m_groups.contains(groupId))
        return;
    m_groupMembers[groupId] = memberNodeIds;
    refreshGroups();
}

void GraphScene::removeGroup(const QString &groupId)
{
    auto it = m_groups.find(groupId);
    if (it == m_groups.end())
        return;
    GroupItem *item = it.value();
    removeItem(item);
    delete item; // scene owns GroupItems
    m_groups.erase(it);
    m_groupMembers.remove(groupId);

    if (m_groups.isEmpty() && m_groupsChangeConnected) {
        disconnect(this, &QGraphicsScene::changed,
                   this, &GraphScene::refreshGroupsFromSceneChange);
        m_groupsChangeConnected = false;
    }
}

QStringList GraphScene::groupIds() const
{
    return m_groups.keys();
}

QSet<QString> GraphScene::groupMembers(const QString &groupId) const
{
    return m_groupMembers.value(groupId);
}

GroupItem *GraphScene::groupItem(const QString &groupId) const
{
    return m_groups.value(groupId, nullptr);
}

void GraphScene::refreshGroupsNow()
{
    refreshGroups();
}

void GraphScene::refreshGroupsFromSceneChange(const QList<QRectF> &)
{
    // changed() fires on every item invalidation (hover, selection, repaints)
    // — far more often than members actually move. Coalesce to at most one
    // geometric refresh per event-loop turn.
    if (m_refreshingGroups || m_groupRefreshQueued || m_groups.isEmpty())
        return;
    m_groupRefreshQueued = true;
    QMetaObject::invokeMethod(this, [this] {
        m_groupRefreshQueued = false;
        refreshGroups();
    }, Qt::QueuedConnection);
}

void GraphScene::refreshGroups()
{
    if (m_groups.isEmpty())
        return;

    m_refreshingGroups = true;
    for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
        const QString &gid = it.key();
        GroupItem *item = it.value();
        const QSet<QString> &memberIds = m_groupMembers.value(gid);

        QRectF unionRect;
        bool first = true;
        for (const QString &mid : memberIds) {
            IGraphNode *n = m_nodes.value(mid, nullptr);
            if (!n)
                continue;
            QRectF r = n->nodeBoundingRect();
            if (first) { unionRect = r; first = false; }
            else        { unionRect = unionRect.united(r); }
        }

        QRectF padded;
        if (!first && !unionRect.isNull()) {
            const qreal p = item->style().padding;
            padded = unionRect.adjusted(-p, -p, p, p);
        }
        item->setBounds(padded);
    }
    m_refreshingGroups = false;
}

void GraphScene::applyLayout(const LayoutResult &result)
{
    for (auto it = result.nodePositions.constBegin();
         it != result.nodePositions.constEnd(); ++it) {
        if (auto *node = nodeForId(it.key()))
            node->graphicsItem()->setPos(it.value());
    }
    // Deliver routing hints before adjusting so the recomputed paths see them.
    for (auto it = result.edgeWaypoints.constBegin();
         it != result.edgeWaypoints.constEnd(); ++it) {
        if (auto *edge = edgeBetween(it.key().first, it.key().second))
            edge->setWaypoints(it.value());
    }
    adjustAllEdges();
    // Groups derive from member positions; refresh synchronously so
    // callers that query bounds right after applyLayout() see fresh data
    // instead of waiting for the next scene->changed emission.
    refreshGroups();
}

void GraphScene::adjustEdgesForNode(const QString &nodeId)
{
    for (auto it = m_edgesByNode.constFind(nodeId);
         it != m_edgesByNode.constEnd() && it.key() == nodeId; ++it)
        it.value()->endpointsChanged();
}

void GraphScene::adjustAllEdges()
{
    for (auto *edge : m_edges)
        edge->endpointsChanged();
}

// Tool dispatch — forward events to active tool if set, but also let the
// base class run so Qt's built-in hover/cursor dispatch keeps working.
// Without calling the base, QGraphicsItem::setCursor() wouldn't take effect
// on hover because the cursor update is driven from the base mouseMoveEvent.
void GraphScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    if (focusItem()) {
        QGraphicsScene::mousePressEvent(e);
        return;
    }

    // Edge-action affordance takes precedence over the active tool.
    if (const auto hit = edgeActionAt(e->scenePos()); hit.edge) {
        mousePressEventEdgeAction(hit.edge, hit.action.id, e->scenePos());
        emit edgeActionTriggered(hit.edge, hit.action.id, e->scenePos());
        e->accept();
        return;
    }

    // Node sub-item hit-test. Unlike the edge-action affordance above,
    // this does NOT return/consume the event — it fires the virtual and
    // signal, then falls through to ordinary tool routing so selection/
    // drag on the same click still work.
    if (const auto hit = nodeSubItemAt(e->scenePos()); hit.node) {
        mousePressEventNodeSubItem(hit.node, hit.subItemId, e->scenePos());
        emit nodeSubItemClicked(hit.node, hit.subItemId, e->scenePos());
    }

    if (m_activeTool)
        m_activeTool->mousePressEvent(e);
    else
        QGraphicsScene::mousePressEvent(e);
}

void GraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    if (focusItem()) {
        QGraphicsScene::mouseMoveEvent(e);
        return;
    }

    if (m_activeTool) {
        m_activeTool->mouseMoveEvent(e);
        // Only run hover/cursor tracking when no mouse button is pressed.
        // While buttons are held, the active tool owns the drag.
        if (e->buttons() == Qt::NoButton)
            QGraphicsScene::mouseMoveEvent(e);
    } else {
        QGraphicsScene::mouseMoveEvent(e);
    }
}

void GraphScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    if (focusItem()) {
        QGraphicsScene::mouseReleaseEvent(e);
        return;
    }

    if (m_activeTool)
        m_activeTool->mouseReleaseEvent(e);
    else
        QGraphicsScene::mouseReleaseEvent(e);
}

void GraphScene::wheelEvent(QGraphicsSceneWheelEvent *e)
{
    if (m_activeTool) {
        m_activeTool->wheelEvent(e);
        if (!e->isAccepted())
            QGraphicsScene::wheelEvent(e);
    } else {
        QGraphicsScene::wheelEvent(e);
    }
}

void GraphScene::keyPressEvent(QKeyEvent *e)
{
    // If any item has scene focus (typically a QGraphicsTextItem being
    // edited in-place inside a node card), let the base class route the
    // event to it — QGraphicsScene::keyPressEvent() forwards the key to
    // focusItem() before falling through. This way tool-level handling
    // (Delete/Backspace, R, Escape, ...) cannot consume keystrokes the
    // user is typing into a focused editor, so consumers no longer have
    // to special-case focusItem() in their own scene subclass.
    if (focusItem())
        QGraphicsScene::keyPressEvent(e);
    else if (m_activeTool)
        m_activeTool->keyPressEvent(e);
    else
        QGraphicsScene::keyPressEvent(e);
}

void GraphScene::keyReleaseEvent(QKeyEvent *e)
{
    // See keyPressEvent() above: focused-item editors see their own
    // releases first, otherwise the active tool handles them.
    if (focusItem())
        QGraphicsScene::keyReleaseEvent(e);
    else if (m_activeTool)
        m_activeTool->keyReleaseEvent(e);
    else
        QGraphicsScene::keyReleaseEvent(e);
}

void GraphScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e)
{
    // The "empty background" heuristic is centralized in
    // isBackgroundDoubleClickPoint(). Do NOT duplicate this test in
    // consumer mouseDoubleClickEvent overrides — connect to the
    // backgroundDoubleClicked() signal or override the protected
    // mouseDoubleClickEventBackground() hook instead.
    if (isBackgroundDoubleClickPoint(e->scenePos())) {
        // Virtual first: subclasses run synchronous work (e.g.,
        // create + place a new node) before observers are notified.
        // Signal second: subscribers see the post-subclass state.
        mouseDoubleClickEventBackground(e->scenePos());
        emit backgroundDoubleClicked(e->scenePos());
        e->accept();
        return;
    }
    QGraphicsScene::mouseDoubleClickEvent(e);
}

bool GraphScene::isBackgroundDoubleClickPoint(const QPointF &scenePos) const
{
    const auto hits = items(scenePos);
    for (auto *item : hits) {
        if (!item) continue;
        if (item->data(kTransientSceneItemRole).toBool())
            continue;
        if (item->acceptedMouseButtons() == Qt::NoButton)
            continue;
        return false;
    }
    return true;
}

void GraphScene::placeNodeAt(const QPointF &)
{
    // Default: do nothing. Subclasses override to create nodes at explicit coords.
}

void GraphScene::mouseDoubleClickEventBackground(const QPointF &)
{
    // Default: do nothing. Subclasses override to react synchronously
    // to background double-clicks. See the header docs for the contract;
    // preferred over a custom mouseDoubleClickEvent override because
    // the scene's empty-area heuristic is centralized in
    // isBackgroundDoubleClickPoint().
}

void GraphScene::mousePressEventEdgeAction(IGraphEdge *, const QString &,
                                           const QPointF &)
{
    // Default: no-op.  Subclasses can react synchronously; the signal
    // edgeActionTriggered() is emitted after this returns.
}

void GraphScene::mousePressEventNodeSubItem(IGraphNode *, const QString &,
                                            const QPointF &)
{
    // Default: no-op.  Subclasses can react synchronously; the signal
    // nodeSubItemClicked() is emitted after this returns. The triggering
    // press is NOT consumed by the scene either way.
}

namespace {
static constexpr qreal kEdgeActionHitRadius = 8.0;
}

GraphScene::EdgeActionHit
GraphScene::edgeActionAt(const QPointF &scenePos) const
{
    // Deterministic winner among overlapping affordances (m_edges is a QHash).
    EdgeActionHit best;
    for (auto *edge : m_edges) {
        for (const EdgeAction &act : edge->edgeActions()) {
            // Obtain the edge's path geometry via its graphics item.
            if (auto *gi = edge->graphicsItem()) {
                if (auto *pathItem = dynamic_cast<QGraphicsPathItem *>(gi)) {
                    const QPainterPath &path = pathItem->path();
                    if (!path.isEmpty()) {
                        // path points are item-local; hit-testing is in scene
                        // coordinates (nodeSubItemAt does the same).
                        const QPointF pt = pathItem->mapToScene(
                            path.pointAtPercent(qBound(0.0, act.pos, 1.0)));
                        if (QLineF(pt, scenePos).length() <= kEdgeActionHitRadius
                            && (!best.edge || edge->edgeId() < best.edge->edgeId())) {
                            best = {edge, act};
                        }
                    }
                }
            }
        }
    }
    return best;
}

GraphScene::NodeSubItemHit
GraphScene::nodeSubItemAt(const QPointF &scenePos) const
{
    // m_nodes is a QHash, so iteration order varies run to run. Overlapping
    // nodes therefore picked an unspecified winner; the smallest node id wins
    // deterministically instead.
    NodeSubItemHit best;
    for (auto *node : m_nodes) {
        if (!node->nodeBoundingRect().contains(scenePos))
            continue;
        const QString subItemId = node->subItemIdAt(scenePos);
        if (subItemId.isEmpty())
            continue;
        if (!best.node || node->nodeId() < best.node->nodeId())
            best = {node, subItemId};
    }
    return best;
}

} // namespace Graffodil
