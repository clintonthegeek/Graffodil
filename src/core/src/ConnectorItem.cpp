#include <graffodil/ConnectorItem.h>
#include <QPen>

namespace Graffodil {

ConnectorItem::ConnectorItem(const QPointF &pos, qreal radius)
    : QGraphicsEllipseItem(-radius, -radius, radius * 2, radius * 2)
{
    setPos(pos);
    setZValue(7);
    setFlag(ItemIsSelectable, false);
    setFlag(ItemIsMovable, false);
    setPen(QPen(QColor(100, 100, 100), 1.5));
    setBrush(QColor(150, 150, 150));
}

} // namespace Graffodil
