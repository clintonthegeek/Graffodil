#pragma once

#include <graffodil/Types.h>
#include <QGraphicsItem>
#include <QPointF>
#include <QRectF>
#include <QString>

namespace Graffodil {

/// Anything that can participate as a node in a GraphScene.
/// Consumers implement this on their own QGraphicsItem subclasses.
class IGraphNode {
public:
    virtual ~IGraphNode() = default;

    /// Unique identifier for this node within the scene.
    virtual QString nodeId() const = 0;

    /// Connection point for the given side. Used by edges and tools
    /// to determine where to attach/draw from.
    virtual QPointF anchorPoint(Side side) const = 0;

    /// Bounding rect in scene coordinates. Used by the scene for
    /// spatial queries and by layout engines for spacing.
    virtual QRectF nodeBoundingRect() const = 0;

    /// Bridge to the Qt graphics item system. The scene calls this
    /// to add/remove the item from the QGraphicsScene.
    /// Must return a stable pointer for the lifetime of the node.
    virtual QGraphicsItem *graphicsItem() = 0;
};

} // namespace Graffodil
