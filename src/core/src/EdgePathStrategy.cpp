#include <graffodil/EdgePathStrategy.h>
#include <cmath>

namespace Graffodil {

// --- DirectPathStrategy ---

QPainterPath DirectPathStrategy::computePath(
    const Anchor &source, const Anchor &target,
    const QList<QPointF> &/*waypoints*/)
{
    QPainterPath path;
    path.moveTo(source.scenePos);
    path.lineTo(target.scenePos);
    return path;
}

// --- BezierPathStrategy ---
// Curvature formula: min(distance * 0.4, 80.0).
// Control points extend along each anchor's outwardDirection.

QPainterPath BezierPathStrategy::computePath(
    const Anchor &source, const Anchor &target,
    const QList<QPointF> &/*waypoints*/)
{
    const double dx = target.scenePos.x() - source.scenePos.x();
    const double dy = target.scenePos.y() - source.scenePos.y();
    const double dist = std::sqrt(dx * dx + dy * dy);
    const double curvature = std::min(dist * 0.4, 80.0);

    const QPointF ctrl1 = source.scenePos + source.outwardDirection * curvature;
    const QPointF ctrl2 = target.scenePos + target.outwardDirection * curvature;

    QPainterPath path;
    path.moveTo(source.scenePos);
    path.cubicTo(ctrl1, ctrl2, target.scenePos);
    return path;
}

// --- OrthogonalPathStrategy ---

static constexpr double kOrthogonalGap = 20.0;

// Is the direction predominantly horizontal (as opposed to vertical)?
static bool isHorizontal(const QPointF &dir)
{
    return std::abs(dir.x()) >= std::abs(dir.y());
}

QPainterPath OrthogonalPathStrategy::computePath(
    const Anchor &source, const Anchor &target,
    const QList<QPointF> &waypoints)
{
    QPainterPath path;
    path.moveTo(source.scenePos);

    if (!waypoints.isEmpty()) {
        QPointF prev = source.scenePos;
        for (const auto &wp : waypoints) {
            path.lineTo(wp.x(), prev.y());
            path.lineTo(wp.x(), wp.y());
            prev = wp;
        }
        path.lineTo(target.scenePos.x(), prev.y());
        path.lineTo(target.scenePos);
    } else {
        const QPointF exitPt  = source.scenePos + source.outwardDirection * kOrthogonalGap;
        const QPointF entryPt = target.scenePos + target.outwardDirection * kOrthogonalGap;

        path.lineTo(exitPt);

        const bool srcHoriz = isHorizontal(source.outwardDirection);
        const bool tgtHoriz = isHorizontal(target.outwardDirection);

        if (srcHoriz) {
            if (tgtHoriz) {
                const double midX = (exitPt.x() + entryPt.x()) / 2.0;
                path.lineTo(midX, exitPt.y());
                path.lineTo(midX, entryPt.y());
            } else {
                path.lineTo(entryPt.x(), exitPt.y());
            }
        } else {
            if (!tgtHoriz) {
                const double midY = (exitPt.y() + entryPt.y()) / 2.0;
                path.lineTo(exitPt.x(), midY);
                path.lineTo(entryPt.x(), midY);
            } else {
                path.lineTo(exitPt.x(), entryPt.y());
            }
        }

        path.lineTo(entryPt);
        path.lineTo(target.scenePos);
    }
    return path;
}

// --- PolylinePathStrategy ---

QPainterPath PolylinePathStrategy::computePath(
    const Anchor &source, const Anchor &target,
    const QList<QPointF> &waypoints)
{
    QPainterPath path;
    path.moveTo(source.scenePos);
    for (const auto &wp : waypoints)
        path.lineTo(wp);
    path.lineTo(target.scenePos);
    return path;
}

} // namespace Graffodil
