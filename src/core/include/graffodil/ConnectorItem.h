#pragma once

#include <QGraphicsEllipseItem>
#include <QPointF>

namespace Graffodil {

/// Small circle drawn at edge waypoints/junctions.
/// Placed at dummy vertex positions and fan-in/fan-out channel points.
class ConnectorItem : public QGraphicsEllipseItem {
public:
    explicit ConnectorItem(const QPointF &pos, qreal radius = 4.5);
};

} // namespace Graffodil
