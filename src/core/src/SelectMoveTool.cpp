#include <graffodil/SelectMoveTool.h>
#include <graffodil/GraphScene.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>

namespace Graffodil {

void SelectMoveTool::activate(GraphScene *scene)
{
    GraphTool::activate(scene);
    emit statusMessage(tr("Click to select, drag to move"));
}

void SelectMoveTool::deactivate()
{
    m_dragNode = nullptr;
    m_dragging = false;
    GraphTool::deactivate();
}

void SelectMoveTool::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_scene)
        return;

    auto *item = m_scene->itemAt(event->scenePos(), QTransform());
    if (!item)
        return;

    // Find the IGraphNode that owns this item
    for (auto *node : m_scene->nodes()) {
        if (node->graphicsItem() == item || node->graphicsItem()->isAncestorOf(item)) {
            m_dragNode = node;
            m_dragStartPos = node->graphicsItem()->pos();
            m_dragging = false;

            if (!(event->modifiers() & Qt::ShiftModifier)) {
                m_scene->clearSelection();
            }
            node->graphicsItem()->setSelected(true);
            return;
        }
    }

    // Check edges
    for (auto *edge : m_scene->edges()) {
        if (edge->graphicsItem() == item || edge->graphicsItem()->isAncestorOf(item)) {
            if (!(event->modifiers() & Qt::ShiftModifier))
                m_scene->clearSelection();
            edge->graphicsItem()->setSelected(true);
            return;
        }
    }

    // Empty space — clear selection
    if (!(event->modifiers() & Qt::ShiftModifier))
        m_scene->clearSelection();
}

void SelectMoveTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_dragNode || !m_scene)
        return;

    m_dragging = true;
    QPointF delta = event->scenePos() - event->lastScenePos();
    QPointF newPos = m_dragNode->graphicsItem()->pos() + delta;
    m_dragNode->graphicsItem()->setPos(newPos);
    m_scene->adjustEdgesForNode(m_dragNode->nodeId());
}

void SelectMoveTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    if (m_dragNode && m_dragging) {
        QPointF newPos = m_dragNode->graphicsItem()->pos();
        if (m_dragStartPos != newPos)
            emit nodeMoved(m_dragNode, m_dragStartPos, newPos);
    }
    m_dragNode = nullptr;
    m_dragging = false;
}

void SelectMoveTool::keyPressEvent(QKeyEvent *event)
{
    if (!m_scene)
        return;

    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        auto nodes = m_scene->selectedNodes();
        auto edges = m_scene->selectedEdges();
        if (!nodes.isEmpty() || !edges.isEmpty())
            emit deleteRequested(nodes, edges);
    }
}

} // namespace Graffodil
