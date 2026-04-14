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
    if (m_node == node && m_visible)
        return;  // no-op — avoids flicker from redundant geometry changes
    m_node = node;
    m_visible = true;
    prepareGeometryChange();
    update();
}

void AnchorHighlight::hideAnchors()
{
    if (!m_visible && m_node == nullptr)
        return;
    m_node = nullptr;
    m_visible = false;
    prepareGeometryChange();
    update();
}

void AnchorHighlight::setActiveAnchor(Side side)
{
    if (m_activeSide == side)
        return;
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

    const QColor accent(0, 120, 215);
    for (auto side : {Side::Top, Side::Right, Side::Bottom, Side::Left}) {
        QPointF pt = m_node->anchorPoint(side);
        if (side == m_activeSide) {
            painter->setPen(QPen(Qt::white, 1.5));
            painter->setBrush(accent);
            painter->drawEllipse(pt, kDotRadius * 1.1, kDotRadius * 1.1);
        } else {
            painter->setPen(QPen(accent, 1.5));
            painter->setBrush(Qt::white);
            painter->drawEllipse(pt, kDotRadius * 0.8, kDotRadius * 0.8);
        }
    }
}

} // namespace Graffodil
