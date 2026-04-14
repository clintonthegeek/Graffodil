#pragma once

#include <graffodil/Types.h>
#include <QPainterPath>
#include <QList>
#include <QPointF>

namespace Graffodil {

/// Pluggable path computation for edges. Determines the shape of the
/// line between two anchors. Subclass for custom routing.
///
/// The anchor's outwardDirection guides strategies that want to exit
/// orthogonally from the node (Bezier control points, Orthogonal bends).
class EdgePathStrategy {
public:
    virtual ~EdgePathStrategy() = default;

    virtual QPainterPath computePath(const Anchor &source,
                                     const Anchor &target,
                                     const QList<QPointF> &waypoints = {}) = 0;
};

/// Straight line from source to target. Ignores waypoints and directions.
class DirectPathStrategy : public EdgePathStrategy {
public:
    QPainterPath computePath(const Anchor &source,
                             const Anchor &target,
                             const QList<QPointF> &waypoints = {}) override;
};

/// Cubic bezier. Curvature = min(distance * 0.4, 80.0). Control points
/// extend along each anchor's outwardDirection.
class BezierPathStrategy : public EdgePathStrategy {
public:
    QPainterPath computePath(const Anchor &source,
                             const Anchor &target,
                             const QList<QPointF> &waypoints = {}) override;
};

/// Right-angle segments with waypoint support. Exits each anchor along
/// its outwardDirection before bending.
class OrthogonalPathStrategy : public EdgePathStrategy {
public:
    QPainterPath computePath(const Anchor &source,
                             const Anchor &target,
                             const QList<QPointF> &waypoints = {}) override;
};

/// Multi-segment straight lines through waypoints.
class PolylinePathStrategy : public EdgePathStrategy {
public:
    QPainterPath computePath(const Anchor &source,
                             const Anchor &target,
                             const QList<QPointF> &waypoints = {}) override;
};

} // namespace Graffodil
