#include <graffodil/GraphScene.h>
#include <graffodil/GraphTool.h>
#include <graffodil/GroupItem.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include <graffodil/GraphEdgeItem.h>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QKeyEvent>
#include <QUuid>

namespace Graffodil {

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
    for (auto *edge : m_edges) {
        if (edge->sourceNode() == node || edge->targetNode() == node)
            connectedIds.append(edge->edgeId());
    }
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
    QList<IGraphEdge *> result;
    for (auto *edge : m_edges) {
        if (edge->sourceNode()->nodeId() == nodeId
            || edge->targetNode()->nodeId() == nodeId)
            result.append(edge);
    }
    return result;
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
    for (auto *edge : m_edges) {
        if (edge->sourceNode()->nodeId() == sourceId
            && edge->targetNode()->nodeId() == targetId) {
            return edge;
        }
    }
    return nullptr;
}

IGraphEdge *GraphScene::edgeBetweenUndirected(const QString &a, const QString &b) const
{
    for (auto *edge : m_edges) {
        const QString &s = edge->sourceNode()->nodeId();
        const QString &t = edge->targetNode()->nodeId();
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

    for (auto *edge : result.edges) {
        removeItem(edge->graphicsItem());
        emit edgeRemoved(edge->edgeId());
    }
    m_edges.clear();

    for (auto *node : result.nodes) {
        removeItem(node->graphicsItem());
        emit nodeRemoved(node->nodeId());
    }
    m_nodes.clear();

    // Groups are scene-owned decorations; clearGraph() removes them too.
    for (auto *g : m_groups) {
        removeItem(g);
        delete g;
    }
    m_groups.clear();
    m_groupMembers.clear();

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

void GraphScene::refreshGroupsFromSceneChange(const QList<QRectF> &)
{
    if (m_refreshingGroups)
        return; // changed() re-emits while we repaint; guard against recursion
    refreshGroups();
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
    adjustAllEdges();
    // Groups derive from member positions; refresh synchronously so
    // callers that query bounds right after applyLayout() see fresh data
    // instead of waiting for the next scene->changed emission.
    refreshGroups();
}

void GraphScene::adjustEdgesForNode(const QString &nodeId)
{
    for (auto *edge : m_edges) {
        if (edge->sourceNode()->nodeId() == nodeId
            || edge->targetNode()->nodeId() == nodeId) {
            if (auto *item = dynamic_cast<GraphEdgeItem *>(edge))
                item->adjust();
        }
    }
}

void GraphScene::adjustAllEdges()
{
    for (auto *edge : m_edges) {
        if (auto *item = dynamic_cast<GraphEdgeItem *>(edge))
            item->adjust();
    }
}

// Tool dispatch — forward events to active tool if set, but also let the
// base class run so Qt's built-in hover/cursor dispatch keeps working.
// Without calling the base, QGraphicsItem::setCursor() wouldn't take effect
// on hover because the cursor update is driven from the base mouseMoveEvent.
void GraphScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    if (m_activeTool)
        m_activeTool->mousePressEvent(e);
    else
        QGraphicsScene::mousePressEvent(e);
}

void GraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
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
    if (m_activeTool)
        m_activeTool->keyPressEvent(e);
    else
        QGraphicsScene::keyPressEvent(e);
}

void GraphScene::keyReleaseEvent(QKeyEvent *e)
{
    if (m_activeTool)
        m_activeTool->keyReleaseEvent(e);
    else
        QGraphicsScene::keyReleaseEvent(e);
}

} // namespace Graffodil
