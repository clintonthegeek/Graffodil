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
        return;
    if (m_nodeGuard)
        disconnect(m_nodeGuard, nullptr, this, nullptr);
    m_node = node;
    m_nodeGuard = node ? dynamic_cast<QGraphicsObject *>(node->graphicsItem()) : nullptr;
    if (m_nodeGuard) {
        // When the node moves, Qt's dirty region is based on the node's own
        // bounding rect. Anchor dots that extend past that rect ghost unless
        // we tell the scene our bounding rect changed too.
        connect(m_nodeGuard, &QGraphicsObject::xChanged, this, [this] {
            prepareGeometryChange();
            update();
        });
        connect(m_nodeGuard, &QGraphicsObject::yChanged, this, [this] {
            prepareGeometryChange();
            update();
        });
    }
    m_visible = true;
    prepareGeometryChange();
    update();
}

void AnchorHighlight::hideAnchors()
{
    if (!m_visible && m_node == nullptr)
        return;
    if (m_nodeGuard)
        disconnect(m_nodeGuard, nullptr, this, nullptr);
    m_node = nullptr;
    m_nodeGuard.clear();
    m_visible = false;
    prepareGeometryChange();
    update();
}

void AnchorHighlight::setActiveAnchor(const QString &anchorId)
{
    if (m_activeAnchorId == anchorId)
        return;
    m_activeAnchorId = anchorId;
    update();
}

QRectF AnchorHighlight::boundingRect() const
{
    if (!m_visible || !m_node || !m_nodeGuard)
        return {};
    QRectF r = m_node->nodeBoundingRect();
    return r.adjusted(-kDotRadius * 2, -kDotRadius * 2,
                      kDotRadius * 2, kDotRadius * 2);
}

void AnchorHighlight::paint(QPainter *painter,
                            const QStyleOptionGraphicsItem * /*option*/,
                            QWidget * /*widget*/)
{
    if (!m_visible || !m_node || !m_nodeGuard)
        return;

    painter->setRenderHint(QPainter::Antialiasing);

    const QColor accent(0, 120, 215);
    const auto anchors = m_node->anchors();
    for (const Anchor &a : anchors) {
        if (a.id == m_activeAnchorId) {
            painter->setPen(QPen(Qt::white, 1.5));
            painter->setBrush(accent);
            painter->drawEllipse(a.scenePos, kDotRadius * 1.1, kDotRadius * 1.1);
        } else {
            painter->setPen(QPen(accent, 1.5));
            painter->setBrush(Qt::white);
            painter->drawEllipse(a.scenePos, kDotRadius * 0.8, kDotRadius * 0.8);
        }
    }
}

} // namespace Graffodil
