#include <graffodil/GraphScene.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>

namespace Graffodil {

void GraphScene::addNode(IGraphNode *node)
{
    if (!node || m_nodes.contains(node->nodeId()))
        return;
    m_nodes.insert(node->nodeId(), node);
    addItem(node->graphicsItem());
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
    addItem(edge->graphicsItem());
    emit edgeAdded(edge);
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
    m_activeTool = tool;
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
}

// Tool dispatch — forward events to active tool if set.
void GraphScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    QGraphicsScene::mousePressEvent(e);
}

void GraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    QGraphicsScene::mouseMoveEvent(e);
}

void GraphScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    QGraphicsScene::mouseReleaseEvent(e);
}

void GraphScene::keyPressEvent(QKeyEvent *e)
{
    QGraphicsScene::keyPressEvent(e);
}

} // namespace Graffodil
