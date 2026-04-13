#pragma once

#include <graffodil/Types.h>
#include <QPainterPath>
#include <QList>
#include <QPointF>

namespace Graffodil {

class EdgePathStrategy {
public:
    virtual ~EdgePathStrategy() = default;
    virtual QPainterPath computePath(
        const QPointF &sourceAnchor, Side sourceSide,
        const QPointF &targetAnchor, Side targetSide,
        const QList<QPointF> &waypoints = {}) = 0;
};

class DirectPathStrategy : public EdgePathStrategy {
public:
    QPainterPath computePath(
        const QPointF &sourceAnchor, Side sourceSide,
        const QPointF &targetAnchor, Side targetSide,
        const QList<QPointF> &waypoints = {}) override;
};

class BezierPathStrategy : public EdgePathStrategy {
public:
    QPainterPath computePath(
        const QPointF &sourceAnchor, Side sourceSide,
        const QPointF &targetAnchor, Side targetSide,
        const QList<QPointF> &waypoints = {}) override;
};

class OrthogonalPathStrategy : public EdgePathStrategy {
public:
    QPainterPath computePath(
        const QPointF &sourceAnchor, Side sourceSide,
        const QPointF &targetAnchor, Side targetSide,
        const QList<QPointF> &waypoints = {}) override;
};

class PolylinePathStrategy : public EdgePathStrategy {
public:
    QPainterPath computePath(
        const QPointF &sourceAnchor, Side sourceSide,
        const QPointF &targetAnchor, Side targetSide,
        const QList<QPointF> &waypoints = {}) override;
};

} // namespace Graffodil
