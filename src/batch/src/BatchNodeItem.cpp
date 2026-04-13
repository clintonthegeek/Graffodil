#include "BatchNodeItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace Graffodil {

BatchNodeItem::BatchNodeItem(QGraphicsItem *parent) : QGraphicsItem(parent)
{
    setZValue(1);
}

void BatchNodeItem::setNodes(const QVector<NodeData> &nodes)
{
    prepareGeometryChange();
    m_nodes = nodes;
    update();
}

void BatchNodeItem::updatePositions(const QVector<QPointF> &positions)
{
    int count = qMin(positions.size(), m_nodes.size());
    for (int i = 0; i < count; ++i)
        m_nodes[i].position = positions[i];
    update();
}

QRectF BatchNodeItem::boundingRect() const
{
    return QRectF(-10000, -10000, 20000, 20000);
}

void BatchNodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    if (m_nodes.isEmpty()) return;

    double lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform());
    if (lod < 0.01) return;

    painter->setPen(Qt::NoPen);

    for (const auto &node : m_nodes) {
        painter->setBrush(node.color);
        if (lod < 0.3)
            painter->fillRect(QRectF(node.position.x() - node.radius, node.position.y() - node.radius,
                                     node.radius * 2, node.radius * 2), painter->brush());
        else
            painter->drawEllipse(node.position, node.radius, node.radius);
    }
}

} // namespace Graffodil
