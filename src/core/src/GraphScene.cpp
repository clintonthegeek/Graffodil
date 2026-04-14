#include <graffodil/GraphScene.h>
#include <graffodil/GraphTool.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include <graffodil/GraphEdgeItem.h>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>

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
    removeItem(node->graphicsItem());
    m_nodes.erase(it);
    emit nodeRemoved(id);
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
    // Re-assign z on items still at the prior defaults.
    const qreal prevNodeZ = above ? DefaultNodeZ : DefaultEdgeZ;  // new meaning
    const qreal prevEdgeZ = above ? DefaultEdgeZ : DefaultNodeZ;
    Q_UNUSED(prevNodeZ);
    Q_UNUSED(prevEdgeZ);
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

QList<IGraphEdge *> GraphScene::edges() const
{
    return m_edges.values();
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

void GraphScene::applyLayout(const LayoutResult &result)
{
    for (auto it = result.nodePositions.constBegin();
         it != result.nodePositions.constEnd(); ++it) {
        if (auto *node = nodeForId(it.key()))
            node->graphicsItem()->setPos(it.value());
    }
    adjustAllEdges();
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

// Tool dispatch — forward events to active tool if set.
void GraphScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    if (m_activeTool)
        m_activeTool->mousePressEvent(e);
    else
        QGraphicsScene::mousePressEvent(e);
}

void GraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    if (m_activeTool)
        m_activeTool->mouseMoveEvent(e);
    else
        QGraphicsScene::mouseMoveEvent(e);
}

void GraphScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    if (m_activeTool)
        m_activeTool->mouseReleaseEvent(e);
    else
        QGraphicsScene::mouseReleaseEvent(e);
}

void GraphScene::keyPressEvent(QKeyEvent *e)
{
    if (m_activeTool)
        m_activeTool->keyPressEvent(e);
    else
        QGraphicsScene::keyPressEvent(e);
}

} // namespace Graffodil
