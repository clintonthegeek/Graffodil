#include <graffodil/AnchorHighlight.h>
#include <graffodil/IGraphNode.h>
#include <QPainter>

namespace Graffodil {

AnchorHighlight::AnchorHighlight()
{
    setZValue(999);
}

void AnchorHighlight::showAnchors(IGraphNode *node)
{
    m_node = node;
    m_visible = true;
    prepareGeometryChange();
    update();
}

void AnchorHighlight::hideAnchors()
{
    m_node = nullptr;
    m_visible = false;
    prepareGeometryChange();
    update();
}

void AnchorHighlight::setActiveAnchor(Side side)
{
    m_activeSide = side;
    update();
}

QRectF AnchorHighlight::boundingRect() const
{
    if (!m_visible || !m_node)
        return {};
    // Encompass all 4 anchor points plus dot radius
    QRectF r = m_node->nodeBoundingRect();
    return r.adjusted(-kDotRadius * 2, -kDotRadius * 2,
                      kDotRadius * 2, kDotRadius * 2);
}

void AnchorHighlight::paint(QPainter *painter,
                             const QStyleOptionGraphicsItem * /*option*/,
                             QWidget * /*widget*/)
{
    if (!m_visible || !m_node)
        return;

    painter->setRenderHint(QPainter::Antialiasing);

    for (auto side : {Side::Top, Side::Right, Side::Bottom, Side::Left}) {
        QPointF pt = m_node->anchorPoint(side);
        if (side == m_activeSide) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(0, 120, 215));
            painter->drawEllipse(pt, kDotRadius, kDotRadius);
        } else {
            painter->setPen(QPen(QColor(0, 120, 215), 1.5));
            painter->setBrush(QColor(0, 120, 215, 80));
            painter->drawEllipse(pt, kDotRadius * 0.7, kDotRadius * 0.7);
        }
    }
}

} // namespace Graffodil
