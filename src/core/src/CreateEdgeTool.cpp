#include <graffodil/CreateEdgeTool.h>
#include <graffodil/GraphScene.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/AnchorHighlight.h>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>
#include <QKeyEvent>
#include <QLineF>

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
        // Click-click mode: the second click landed on the background.
        dropOnEmpty(event->scenePos());
        return;
    }

    if (!m_hasSource) {
        m_hasSource = true;
        m_sourceNodeId = node->nodeId();
        const Anchor a = nearestAnchor(node, event->scenePos());
        m_sourceAnchorId = a.id;
        m_pressScenePos = event->scenePos();
        m_sawDrag = false;

        m_previewLine = m_scene->addLine(QLineF(a.scenePos, a.scenePos),
                                          QPen(QColor(0, 120, 215), 1.5, Qt::DashLine));
        m_previewLine->setZValue(1000);

        if (m_highlight) {
            m_highlight->showAnchors(node);
            m_highlight->setActiveAnchor(m_sourceAnchorId);
        }
    } else {
        commitEdge(node, event->scenePos());
    }
}

void CreateEdgeTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_hasSource
        && (event->scenePos() - m_pressScenePos).manhattanLength() > kDragThreshold)
        m_sawDrag = true;

    updatePreview(event->scenePos());
}

void CreateEdgeTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    IGraphNode *source = sourceNode();
    if (!source) {
        // The source was destroyed mid-gesture: nothing to complete.
        cancel();
        return;
    }

    if (!m_sawDrag) {
        // Drag-only: release without dragging cancels the creation.
        // Otherwise (default): stay in click-click mode and wait for a
        // second click on a target node.
        if (m_dragOnly)
            cancel();
        return;
    }

    IGraphNode *target = findNodeAt(event->scenePos());
    if (!target) {
        dropOnEmpty(event->scenePos());
        return;
    }
    if (target == source) {
        // Self-loop: a rejected gesture, not a drop on empty canvas.
        cancel();
        return;
    }
    commitEdge(target, event->scenePos());
}

void CreateEdgeTool::commitEdge(IGraphNode *target, const QPointF &cursorScenePos)
{
    IGraphNode *source = sourceNode();
    if (target && target != source && source) {
        const Anchor tgt = nearestAnchor(target, cursorScenePos);
        emit edgeRequested(source, m_sourceAnchorId, target, tgt.id);
    }
    cancel();
}

void CreateEdgeTool::dropOnEmpty(const QPointF &cursorScenePos)
{
    IGraphNode *source = sourceNode();
    if (!source)
        return;

    const QString anchorId = m_sourceAnchorId;
    // Reset before emitting: the handler may run a modal menu or add nodes
    // to the scene, and must not find the tool mid-gesture.
    cancel();
    emit edgeDroppedOnEmpty(source, anchorId, cursorScenePos);
}

void CreateEdgeTool::updatePreview(const QPointF &cursorScenePos)
{
    IGraphNode *hovered = findNodeAt(cursorScenePos);
    // Fallback: anchor dots sit on the node's edge, so a pointer that's
    // just outside the node rect (but within the anchor hit radius) still
    // counts as "hovering this node" for highlight purposes. Without this
    // the dots vanish the moment the cursor lands on them.
    if (!hovered && m_scene) {
        qreal best = m_anchorHoverRadius;
        for (auto *node : m_scene->nodes()) {
            for (const Anchor &a : node->anchors()) {
                const qreal d = QLineF(a.scenePos, cursorScenePos).length();
                if (d <= best) {
                    best = d;
                    hovered = node;
                }
            }
        }
    }
    Anchor hoverAnchor;
    if (hovered)
        hoverAnchor = nearestAnchor(hovered, cursorScenePos);

    if (IGraphNode *source = sourceNode()) {
        QPointF end = cursorScenePos;
        if (hovered && hovered != source)
            end = hoverAnchor.scenePos;
        if (m_previewLine) {
            // For swivel sources (empty id), resolve the start dynamically
            // against the current preview endpoint so the line follows the
            // node's boundary instead of collapsing to (0,0).
            QPointF start;
            if (m_sourceAnchorId.isEmpty())
                start = source->anchorToward(end).scenePos;
            else
                start = source->anchor(m_sourceAnchorId).scenePos;
            m_previewLine->setLine(QLineF(start, end));
        }
        if (m_highlight) {
            if (hovered && hovered != source) {
                m_highlight->showAnchors(hovered);
                m_highlight->setActiveAnchor(hoverAnchor.id);
            } else {
                m_highlight->showAnchors(source);
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

IGraphNode *CreateEdgeTool::sourceNode() const
{
    if (!m_hasSource || !m_scene || m_sourceNodeId.isEmpty())
        return nullptr;
    return m_scene->nodeForId(m_sourceNodeId);
}

void CreateEdgeTool::cancel()
{
    m_hasSource = false;
    m_sourceNodeId.clear();
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
