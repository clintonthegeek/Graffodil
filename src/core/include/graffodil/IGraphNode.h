#pragma once

#include <graffodil/Types.h>
#include <QGraphicsItem>
#include <QList>
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

    /// The connection points this node exposes. Consumers control how many
    /// there are and what IDs they use. Use Graffodil::compassAnchors() for
    /// the standard 4-compass layout, or build your own list for named
    /// ports (e.g., "audio_out_l", "sidechain_in").
    virtual QList<Anchor> anchors() const = 0;

    /// Resolve an anchor dynamically based on the opposing endpoint.
    /// Called by edges whose endpoint anchor id is empty (the "swivel"
    /// case — attach to whichever point on the boundary is most
    /// appropriate given where the other end is).
    ///
    /// Default implementation returns the anchor from anchors() whose
    /// scenePos is closest to `other`. Override for genuinely dynamic
    /// nodes (circles, irregular polygons) that compute a boundary
    /// intersection on demand.
    virtual Anchor anchorToward(const QPointF &other) const;

    /// Convenience: find an anchor by id. Returns a default-constructed
    /// Anchor if no match. Non-virtual — implemented in terms of anchors().
    Anchor anchor(const QString &id) const;

    /// Bounding rect in scene coordinates. Used by the scene for
    /// spatial queries and by layout engines for spacing.
    virtual QRectF nodeBoundingRect() const = 0;

    /// Preferred size for layout / auto-sizing passes.
    /// Return an invalid QSizeF (the default) to indicate "no preference —
    /// use whatever nodeBoundingRect() currently reports".
    /// Layout engines or the scene may call this before placing or resizing
    /// a node so that card-like nodes can expose their ideal dimensions
    /// without forcing every consumer to duplicate the same content-measurement
    /// logic.
    virtual QSizeF preferredSize() const { return QSizeF(); }

    /// Optional hook so the scene or a layout engine can ask the node to
    /// adopt a new geometry.  Default is a no-op; nodes that support dynamic
    /// resizing should re-layout internal content and ensure that subsequent
    /// calls to nodeBoundingRect() reflect the requested size.
    virtual void setGeometry(const QRectF &rect) { Q_UNUSED(rect); }

    /// Bridge to the Qt graphics item system. The scene calls this
    /// to add/remove the item from the QGraphicsScene.
    /// Must return a stable pointer for the lifetime of the node.
    virtual QGraphicsItem *graphicsItem() = 0;
};

} // namespace Graffodil
