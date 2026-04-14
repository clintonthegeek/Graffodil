#include <graffodil/SelectMoveTool.h>
#include <graffodil/GraphScene.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <QBrush>
#include <QPen>

namespace Graffodil {

// Resolve the click to an IGraphNode*, IGraphEdge*, or neither.
// Prefers nodes over edges when they overlap.
static IGraphNode *findNodeAt(GraphScene *scene, const QPointF &scenePos)
{
    auto items = scene->items(scenePos);
    for (auto *item : items) {
        for (auto *node : scene->nodes()) {
            if (node->graphicsItem() == item
                || node->graphicsItem()->isAncestorOf(item))
                return node;
        }
    }
    return nullptr;
}

static IGraphEdge *findEdgeAt(GraphScene *scene, const QPointF &scenePos)
{
    auto items = scene->items(scenePos);
    for (auto *item : items) {
        for (auto *edge : scene->edges()) {
            if (edge->graphicsItem() == item
                || edge->graphicsItem()->isAncestorOf(item))
                return edge;
        }
    }
    return nullptr;
}

void SelectMoveTool::activate(GraphScene *scene)
{
    GraphTool::activate(scene);
    emit statusMessage(tr("Click=select, Shift=add, Ctrl=toggle, Del=delete, R=reverse edges"));
}

void SelectMoveTool::deactivate()
{
    m_dragNode = nullptr;
    m_draggedNodes.clear();
    m_dragStartPositions.clear();
    m_dragging = false;
    if (m_rubberBand && m_scene) {
        m_scene->removeItem(m_rubberBand);
        delete m_rubberBand;
        m_rubberBand = nullptr;
    }
    m_rubberBanding = false;
    GraphTool::deactivate();
}

void SelectMoveTool::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_scene)
        return;

    const bool shift = event->modifiers() & Qt::ShiftModifier;
    const bool ctrl  = event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier);
    const bool additive = shift || ctrl;

    m_pressScenePos = event->scenePos();

    // Try node first
    if (auto *node = findNodeAt(m_scene, event->scenePos())) {
        QGraphicsItem *gi = node->graphicsItem();
        const bool wasSelected = gi->isSelected();

        if (ctrl) {
            // Toggle
            gi->setSelected(!wasSelected);
            // Don't start drag on toggle
            m_dragNode = nullptr;
            return;
        }

        if (!additive && !wasSelected)
            m_scene->clearSelection();

        gi->setSelected(true);

        // Start drag — collect all currently selected nodes for group move
        m_dragNode = node;
        m_draggedNodes = m_scene->selectedNodes();
        m_dragStartPositions.clear();
        for (auto *n : m_draggedNodes)
            m_dragStartPositions[n] = n->graphicsItem()->pos();
        m_dragging = false;
        return;
    }

    // Then edge
    if (auto *edge = findEdgeAt(m_scene, event->scenePos())) {
        QGraphicsItem *gi = edge->graphicsItem();
        const bool wasSelected = gi->isSelected();

        if (ctrl) {
            gi->setSelected(!wasSelected);
            return;
        }

        if (!additive)
            m_scene->clearSelection();

        gi->setSelected(true);
        return;
    }

    // Empty space — start rubber-band
    if (!additive)
        m_scene->clearSelection();

    m_rubberBandOrigin = event->scenePos();
    m_rubberBandModifiers = event->modifiers();
    m_rubberBand = new QGraphicsRectItem(QRectF(m_rubberBandOrigin, QSizeF(0, 0)));
    m_rubberBand->setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
    m_rubberBand->setBrush(QBrush(QColor(0, 120, 215, 30)));
    m_rubberBand->setZValue(10000);
    m_scene->addItem(m_rubberBand);
    m_rubberBanding = true;
}

void SelectMoveTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_scene)
        return;

    // Rubber-band
    if (m_rubberBanding && m_rubberBand) {
        QRectF r = QRectF(m_rubberBandOrigin, event->scenePos()).normalized();
        m_rubberBand->setRect(r);
        return;
    }

    // Node drag
    if (m_dragNode && !m_draggedNodes.isEmpty()) {
        QPointF delta = event->scenePos() - event->lastScenePos();
        if (!m_dragging && (event->scenePos() - m_pressScenePos).manhattanLength() < 3)
            return;  // small threshold to avoid tiny accidental moves
        m_dragging = true;
        for (auto *n : m_draggedNodes) {
            n->graphicsItem()->setPos(n->graphicsItem()->pos() + delta);
            m_scene->adjustEdgesForNode(n->nodeId());
        }
    }
}

void SelectMoveTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_scene)
        return;

    // Finalize rubber-band — selects nodes only.
    // Edges are deliberately excluded: they're selected via click or
    // Shift/Ctrl+click, never by rectangle, because their elongated shapes
    // make rect-intersection selection feel arbitrary.
    if (m_rubberBanding && m_rubberBand) {
        QRectF r = m_rubberBand->rect();
        const bool additive = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::MetaModifier);
        if (!additive)
            m_scene->clearSelection();
        for (auto *node : m_scene->nodes()) {
            if (r.contains(node->graphicsItem()->sceneBoundingRect().center()))
                node->graphicsItem()->setSelected(true);
        }

        m_scene->removeItem(m_rubberBand);
        delete m_rubberBand;
        m_rubberBand = nullptr;
        m_rubberBanding = false;
        return;
    }

    // Finalize drag
    if (m_dragNode && m_dragging) {
        for (auto *n : m_draggedNodes) {
            QPointF oldPos = m_dragStartPositions.value(n);
            QPointF newPos = n->graphicsItem()->pos();
            if (oldPos != newPos)
                emit nodeMoved(n, oldPos, newPos);
        }
    }
    m_dragNode = nullptr;
    m_draggedNodes.clear();
    m_dragStartPositions.clear();
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
    } else if (event->key() == Qt::Key_R) {
        auto edges = m_scene->selectedEdges();
        if (!edges.isEmpty())
            emit reverseRequested(edges);
    }
}

} // namespace Graffodil
