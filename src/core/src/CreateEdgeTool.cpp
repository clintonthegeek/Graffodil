#include <graffodil/CreateEdgeTool.h>
#include <graffodil/GraphScene.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/AnchorHighlight.h>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>
#include <QKeyEvent>

namespace Graffodil {

static constexpr qreal kDragThreshold = 4.0;

CreateEdgeTool::CreateEdgeTool(QObject *parent)
    : GraphTool(parent)
    , m_highlight(new AnchorHighlight)
{
    // AnchorHighlight is allocated for the tool's lifetime.
}

CreateEdgeTool::~CreateEdgeTool()
{
    // If the scene was destroyed first, QPointer has already been cleared
    // and we have nothing to clean up. Otherwise take the item back from
    // the scene (if still attached) and delete it ourselves.
    if (!m_highlight)
        return;
    if (m_highlight->scene())
        m_highlight->scene()->removeItem(m_highlight);
    delete m_highlight;
}

void CreateEdgeTool::activate(GraphScene *scene)
{
    GraphTool::activate(scene);
    if (m_highlight && m_highlight->scene() != scene)
        scene->addItem(m_highlight);
    if (m_highlight)
        m_highlight->hideAnchors();
    emit statusMessage(tr("Click or drag from source node to target node. Esc to cancel."));
}

void CreateEdgeTool::deactivate()
{
    cancel();
    if (m_highlight && m_highlight->scene())
        m_highlight->scene()->removeItem(m_highlight);
    GraphTool::deactivate();
}

IGraphNode *CreateEdgeTool::findNodeAt(const QPointF &scenePos) const
{
    if (!m_scene) return nullptr;
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

Anchor CreateEdgeTool::nearestAnchor(IGraphNode *node, const QPointF &scenePos) const
{
    // Pick whichever of the node's anchors is closest to the cursor.
    // For swivel nodes (where anchorToward is overridden), this still
    // picks from the static list — so swivel nodes may want to include
    // a single synthetic anchor in their anchors() list that
    // anchorToward can refine.
    return node->anchorToward(scenePos);
}

void CreateEdgeTool::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_scene)
        return;

    IGraphNode *node = findNodeAt(event->scenePos());

    if (!node) {
        cancel();
        return;
    }

    if (!m_sourceNode) {
        m_sourceNode = node;
        const Anchor a = nearestAnchor(node, event->scenePos());
        m_sourceAnchorId = a.id;
        m_pressScenePos = event->scenePos();
        m_sawDrag = false;

        m_previewLine = m_scene->addLine(QLineF(a.scenePos, a.scenePos),
                                          QPen(QColor(0, 120, 215), 1.5, Qt::DashLine));
        m_previewLine->setZValue(1000);

        if (m_highlight) {
            m_highlight->showAnchors(m_sourceNode);
            m_highlight->setActiveAnchor(m_sourceAnchorId);
        }
    } else {
        commitEdge(node, event->scenePos());
    }
}

void CreateEdgeTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_sourceNode
        && (event->scenePos() - m_pressScenePos).manhattanLength() > kDragThreshold)
        m_sawDrag = true;

    updatePreview(event->scenePos());
}

void CreateEdgeTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_sourceNode || event->button() != Qt::LeftButton)
        return;

    if (!m_sawDrag) {
        // Drag-only: release without dragging cancels the creation.
        // Otherwise (default): stay in click-click mode and wait for a
        // second click on a target node.
        if (m_dragOnly)
            cancel();
        return;
    }

    IGraphNode *target = findNodeAt(event->scenePos());
    if (!target || target == m_sourceNode) {
        cancel();
        return;
    }
    commitEdge(target, event->scenePos());
}

void CreateEdgeTool::commitEdge(IGraphNode *target, const QPointF &cursorScenePos)
{
    if (target && target != m_sourceNode) {
        const Anchor tgt = nearestAnchor(target, cursorScenePos);
        emit edgeRequested(m_sourceNode, m_sourceAnchorId, target, tgt.id);
    }
    cancel();
}

void CreateEdgeTool::updatePreview(const QPointF &cursorScenePos)
{
    IGraphNode *hovered = findNodeAt(cursorScenePos);
    Anchor hoverAnchor;
    if (hovered)
        hoverAnchor = nearestAnchor(hovered, cursorScenePos);

    if (m_sourceNode) {
        QPointF end = cursorScenePos;
        if (hovered && hovered != m_sourceNode)
            end = hoverAnchor.scenePos;
        if (m_previewLine) {
            // For swivel sources (empty id), resolve the start dynamically
            // against the current preview endpoint so the line follows the
            // node's boundary instead of collapsing to (0,0).
            QPointF start;
            if (m_sourceAnchorId.isEmpty())
                start = m_sourceNode->anchorToward(end).scenePos;
            else
                start = m_sourceNode->anchor(m_sourceAnchorId).scenePos;
            m_previewLine->setLine(QLineF(start, end));
        }
        if (m_highlight) {
            if (hovered && hovered != m_sourceNode) {
                m_highlight->showAnchors(hovered);
                m_highlight->setActiveAnchor(hoverAnchor.id);
            } else {
                m_highlight->showAnchors(m_sourceNode);
                m_highlight->setActiveAnchor(m_sourceAnchorId);
            }
        }
    } else {
        if (m_highlight) {
            if (hovered) {
                m_highlight->showAnchors(hovered);
                m_highlight->setActiveAnchor(hoverAnchor.id);
            } else {
                m_highlight->hideAnchors();
            }
        }
    }
}

void CreateEdgeTool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        cancel();
}

void CreateEdgeTool::setDragOnly(bool dragOnly)
{
    m_dragOnly = dragOnly;
}

void CreateEdgeTool::cancel()
{
    m_sourceNode = nullptr;
    m_sourceAnchorId.clear();
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
