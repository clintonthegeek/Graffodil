#pragma once

#include <QHash>
#include <QList>
#include <QPointF>
#include <QPair>
#include <QString>

namespace Graffodil {

enum class Side { Top, Right, Bottom, Left };

enum class ArrowEnd { Source, Target };

/// Communication contract between layout engines and the scene.
/// Layout engines produce this; the scene consumes it via applyLayout().
struct LayoutResult {
    QHash<QString, QPointF> nodePositions;
    /// Optional waypoints for edges spanning multiple layers (Sugiyama)
    /// or needing routing hints. Key is (sourceId, targetId).
    QHash<QPair<QString, QString>, QList<QPointF>> edgeWaypoints;
};

} // namespace Graffodil
