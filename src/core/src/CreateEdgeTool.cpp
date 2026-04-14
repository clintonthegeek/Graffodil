#include <graffodil/CreateEdgeTool.h>
#include <graffodil/GraphScene.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/AnchorHighlight.h>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>
#include <QKeyEvent>

namespace Graffodil {

static constexpr qreal kDragThreshold = 4.0;

void CreateEdgeTool::activate(GraphScene *scene)
{
    GraphTool::activate(scene);
    m_highlight = new AnchorHighlight();
    scene->addItem(m_highlight);
    emit statusMessage(tr("Click or drag from source node to target node. Esc to cancel."));
}

void CreateEdgeTool::deactivate()
{
    cancel();
    if (m_highlight && m_scene) {
        m_scene->removeItem(m_highlight);
        delete m_highlight;
        m_highlight = nullptr;
    }
    GraphTool::deactivate();
}

IGraphNode *CreateEdgeTool::findNodeAt(const QPointF &scenePos) const
{
    if (!m_scene) return nullptr;
    // items() returns the stack at the point — walk all of them so we
    // can still find the node when an edge sits on top.
    const auto items = m_scene->items(scenePos);
    for (auto *item : items) {
        for (auto *node : m_scene->nodes()) {
            if (node->graphicsItem() == item
                || node->graphicsItem()->isAncestorOf(item))
                return node;
        }
    }
    return nullptr;
}

Side CreateEdgeTool::nearestSide(IGraphNode *node, const QPointF &scenePos) const
{
    Side best = Side::Right;
    qreal bestDist = std::numeric_limits<qreal>::max();
    for (auto side : {Side::Top, Side::Right, Side::Bottom, Side::Left}) {
        qreal d = QLineF(node->anchorPoint(side), scenePos).length();
        if (d < bestDist) {
            bestDist = d;
            best = side;
        }
    }
    return best;
}

void CreateEdgeTool::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_scene)
        return;

    IGraphNode *node = findNodeAt(event->scenePos());

    if (!node) {
        // Click on empty space cancels any in-progress edge and clears state.
        cancel();
        return;
    }

    if (!m_sourceNode) {
        // First interaction with a node: mark source. This serves both the
        // click-click flow (source set now, target set by next click) and
        // the drag flow (source set now, target set by release after drag).
        m_sourceNode = node;
        m_sourceSide = nearestSide(node, event->scenePos());
        m_pressScenePos = event->scenePos();
        m_sawDrag = false;

        QPointF start = m_sourceNode->anchorPoint(m_sourceSide);
        m_previewLine = m_scene->addLine(QLineF(start, start),
                                          QPen(QColor(0, 120, 215), 1.5, Qt::DashLine));
        m_previewLine->setZValue(1000);

        if (m_highlight)
            m_highlight->showAnchors(m_sourceNode);
    } else {
        // Second click of a click-click flow: try to commit against this node.
        commitEdge(node, event->scenePos());
    }
}

void CreateEdgeTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // Track drag progression for drag-to-create.
    if (m_sourceNode
        && (event->scenePos() - m_pressScenePos).manhattanLength() > kDragThreshold)
        m_sawDrag = true;

    // Always update — so hover over a node shows which anchor will be chosen
    // before any click.
    updatePreview(event->scenePos());
}

void CreateEdgeTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_sourceNode || event->button() != Qt::LeftButton)
        return;

    // Only commit on release if the user actually dragged. A click-without-
    // drag falls through to click-click mode (wait for second click).
    if (!m_sawDrag)
        return;

    IGraphNode *target = findNodeAt(event->scenePos());
    if (!target || target == m_sourceNode) {
        // Dragged to empty space or back to source — cancel rather than commit.
        cancel();
        return;
    }
    commitEdge(target, event->scenePos());
}

void CreateEdgeTool::commitEdge(IGraphNode *target, const QPointF &cursorScenePos)
{
    if (target && target != m_sourceNode) {
        Side targetSide = nearestSide(target, cursorScenePos);
        emit edgeRequested(m_sourceNode, m_sourceSide, target, targetSide);
    }
    cancel();
}

void CreateEdgeTool::updatePreview(const QPointF &cursorScenePos)
{
    IGraphNode *hovered = findNodeAt(cursorScenePos);
    Side hoveredSide = hovered ? nearestSide(hovered, cursorScenePos) : Side::Right;

    if (m_sourceNode) {
        // In-progress edge: snap preview endpoint to hovered target anchor.
        QPointF end = cursorScenePos;
        if (hovered && hovered != m_sourceNode)
            end = hovered->anchorPoint(hoveredSide);
        if (m_previewLine) {
            const QPointF start = m_sourceNode->anchorPoint(m_sourceSide);
            m_previewLine->setLine(QLineF(start, end));
        }
        if (m_highlight) {
            if (hovered && hovered != m_sourceNode) {
                m_highlight->showAnchors(hovered);
                m_highlight->setActiveAnchor(hoveredSide);
            } else {
                m_highlight->showAnchors(m_sourceNode);
                m_highlight->setActiveAnchor(m_sourceSide);
            }
        }
    } else {
        // Pre-click hover: show which anchor will be the source when clicked.
        if (m_highlight) {
            if (hovered) {
                m_highlight->showAnchors(hovered);
                m_highlight->setActiveAnchor(hoveredSide);
            } else {
                m_highlight->hideAnchors();
            }
        }
    }
}

void CreateEdgeTool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void CreateEdgeTool::cancel()
{
    m_sourceNode = nullptr;
    m_sawDrag = false;
    if (m_previewLine && m_scene) {
        m_scene->removeItem(m_previewLine);
        delete m_previewLine;
        m_previewLine = nullptr;
    }
    if (m_highlight)
        m_highlight->hideAnchors();
}

} // namespace Graffodil
