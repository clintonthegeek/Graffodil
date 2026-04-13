#include <graffodil/EdgePathStrategy.h>
#include <cmath>

namespace Graffodil {

// --- DirectPathStrategy ---

QPainterPath DirectPathStrategy::computePath(
    const QPointF &sourceAnchor, Side /*sourceSide*/,
    const QPointF &targetAnchor, Side /*targetSide*/,
    const QList<QPointF> &/*waypoints*/)
{
    QPainterPath path;
    path.moveTo(sourceAnchor);
    path.lineTo(targetAnchor);
    return path;
}

// --- BezierPathStrategy ---
// Curvature formula: min(distance * 0.4, 80.0).
// Control points extend outward from connection side.

static QPointF controlPointOffset(Side side, double curvature)
{
    switch (side) {
    case Side::Right:  return {curvature, 0};
    case Side::Left:   return {-curvature, 0};
    case Side::Bottom: return {0, curvature};
    case Side::Top:    return {0, -curvature};
    }
    Q_UNREACHABLE();
}

QPainterPath BezierPathStrategy::computePath(
    const QPointF &sourceAnchor, Side sourceSide,
    const QPointF &targetAnchor, Side targetSide,
    const QList<QPointF> &/*waypoints*/)
{
    const double dx = targetAnchor.x() - sourceAnchor.x();
    const double dy = targetAnchor.y() - sourceAnchor.y();
    const double dist = std::sqrt(dx * dx + dy * dy);
    const double curvature = std::min(dist * 0.4, 80.0);

    const QPointF ctrl1 = sourceAnchor + controlPointOffset(sourceSide, curvature);
    const QPointF ctrl2 = targetAnchor + controlPointOffset(targetSide, curvature);

    QPainterPath path;
    path.moveTo(sourceAnchor);
    path.cubicTo(ctrl1, ctrl2, targetAnchor);
    return path;
}

// --- OrthogonalPathStrategy ---

static constexpr double kOrthogonalGap = 20.0;

QPainterPath OrthogonalPathStrategy::computePath(
    const QPointF &sourceAnchor, Side sourceSide,
    const QPointF &targetAnchor, Side targetSide,
    const QList<QPointF> &waypoints)
{
    QPainterPath path;
    path.moveTo(sourceAnchor);

    if (!waypoints.isEmpty()) {
        QPointF prev = sourceAnchor;
        for (const auto &wp : waypoints) {
            path.lineTo(wp.x(), prev.y());
            path.lineTo(wp.x(), wp.y());
            prev = wp;
        }
        path.lineTo(targetAnchor.x(), prev.y());
        path.lineTo(targetAnchor);
    } else {
        QPointF exitPt = sourceAnchor + controlPointOffset(sourceSide, kOrthogonalGap);
        QPointF entryPt = targetAnchor + controlPointOffset(targetSide, kOrthogonalGap);

        path.lineTo(exitPt);

        if (sourceSide == Side::Right || sourceSide == Side::Left) {
            if (targetSide == Side::Left || targetSide == Side::Right) {
                double midX = (exitPt.x() + entryPt.x()) / 2.0;
                path.lineTo(midX, exitPt.y());
                path.lineTo(midX, entryPt.y());
            } else {
                path.lineTo(entryPt.x(), exitPt.y());
            }
        } else {
            if (targetSide == Side::Top || targetSide == Side::Bottom) {
                double midY = (exitPt.y() + entryPt.y()) / 2.0;
                path.lineTo(exitPt.x(), midY);
                path.lineTo(entryPt.x(), midY);
            } else {
                path.lineTo(exitPt.x(), entryPt.y());
            }
        }

        path.lineTo(entryPt);
        path.lineTo(targetAnchor);
    }
    return path;
}

// --- PolylinePathStrategy ---

QPainterPath PolylinePathStrategy::computePath(
    const QPointF &sourceAnchor, Side /*sourceSide*/,
    const QPointF &targetAnchor, Side /*targetSide*/,
    const QList<QPointF> &waypoints)
{
    QPainterPath path;
    path.moveTo(sourceAnchor);
    for (const auto &wp : waypoints)
        path.lineTo(wp);
    path.lineTo(targetAnchor);
    return path;
}

} // namespace Graffodil
