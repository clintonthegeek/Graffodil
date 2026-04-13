#include <graffodil/CreateEdgeTool.h>
#include <graffodil/GraphScene.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/AnchorHighlight.h>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>
#include <QKeyEvent>

namespace Graffodil {

void CreateEdgeTool::activate(GraphScene *scene)
{
    GraphTool::activate(scene);
    m_highlight = new AnchorHighlight();
    scene->addItem(m_highlight);
    emit statusMessage(tr("Click source node, then target node"));
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
    auto *item = m_scene->itemAt(scenePos, QTransform());
    if (!item) return nullptr;
    for (auto *node : m_scene->nodes()) {
        if (node->graphicsItem() == item || node->graphicsItem()->isAncestorOf(item))
            return node;
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
        cancel();
        return;
    }

    if (!m_sourceNode) {
        // First click: select source
        m_sourceNode = node;
        m_sourceSide = nearestSide(node, event->scenePos());

        QPointF start = m_sourceNode->anchorPoint(m_sourceSide);
        m_previewLine = m_scene->addLine(QLineF(start, start),
                                          QPen(Qt::gray, 1.5, Qt::DashLine));
        m_previewLine->setZValue(1000);

        if (m_highlight)
            m_highlight->showAnchors(m_sourceNode);
    } else {
        // Second click: try to create edge
        if (node != m_sourceNode) {
            Side targetSide = nearestSide(node, event->scenePos());
            emit edgeRequested(m_sourceNode, m_sourceSide, node, targetSide);
        }
        cancel();
    }
}

void CreateEdgeTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_previewLine && m_sourceNode) {
        QPointF start = m_sourceNode->anchorPoint(m_sourceSide);
        m_previewLine->setLine(QLineF(start, event->scenePos()));
    }

    // Update highlight for hovered node
    if (m_highlight && m_scene) {
        IGraphNode *hovered = findNodeAt(event->scenePos());
        if (hovered && hovered != m_sourceNode) {
            m_highlight->showAnchors(hovered);
            m_highlight->setActiveAnchor(nearestSide(hovered, event->scenePos()));
        } else if (!m_sourceNode || hovered == m_sourceNode) {
            if (m_sourceNode)
                m_highlight->showAnchors(m_sourceNode);
            else
                m_highlight->hideAnchors();
        }
    }
}

void CreateEdgeTool::mouseReleaseEvent(QGraphicsSceneMouseEvent * /*event*/)
{
    // Two-click mode — nothing on release
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
    if (m_previewLine && m_scene) {
        m_scene->removeItem(m_previewLine);
        delete m_previewLine;
        m_previewLine = nullptr;
    }
    if (m_highlight)
        m_highlight->hideAnchors();
}

} // namespace Graffodil
