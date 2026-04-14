#pragma once

#include <graffodil/Types.h>
#include <QList>
#include <QRectF>

namespace Graffodil {

/// Conventional anchor id strings for compass anchors. Consumers can use
/// anything they want, but these are the defaults produced by
/// compassAnchors() and assumed by CompassAnchorId::* constants below.
namespace AnchorIds {
constexpr auto Top    = "top";
constexpr auto Right  = "right";
constexpr auto Bottom = "bottom";
constexpr auto Left   = "left";
} // namespace AnchorIds

/// Build the standard 4 compass anchors from a rect (in scene coords).
/// Each anchor's outwardDirection is the unit normal pointing away from
/// the rect center.
QList<Anchor> compassAnchors(const QRectF &sceneRect);

} // namespace Graffodil
