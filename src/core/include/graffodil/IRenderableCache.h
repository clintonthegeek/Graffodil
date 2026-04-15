#pragma once

#include <QPixmap>
#include <QSize>

namespace Graffodil {

/// Optional capability interface for IGraphNode implementers whose paint()
/// is expensive (markdown, thumbnails, composed glyphs, etc.).
///
/// Batch mode (Graffodil::Batch) queries every node via dynamic_cast to
/// IRenderableCache*. When a node provides one, BatchNodeItem composites
/// the returned pixmap instead of drawing its default ellipse. Nodes that
/// do not implement this interface are drawn with the default primitive —
/// there is no behaviour change for existing consumers.
///
/// Implementers are responsible for caching. The Batch layer calls
/// cachedPixmap() each time it synchronises; implementations should
/// return a stored pixmap and regenerate only when content changes,
/// invalidating via invalidateCache() from the consumer side.
class IRenderableCache {
public:
    virtual ~IRenderableCache() = default;

    /// Return a pixmap representation sized for `requestedSize`.
    /// Implementations may return a pixmap of any size; the Batch layer
    /// scales to `contentSize()` at paint time.
    virtual QPixmap cachedPixmap(const QSize &requestedSize) = 0;

    /// Logical size the pixmap should be drawn at, in scene units.
    /// Distinct from the node's boundingRect when the node paints with
    /// padding, margins, or scaling.
    virtual QSize contentSize() const = 0;

    /// Cache-invalidation hook. Consumers call this when the underlying
    /// content changes (e.g. markdown text edited, thumbnail regenerated).
    /// Implementations typically clear their stored QPixmap so the next
    /// cachedPixmap() call rebuilds it.
    virtual void invalidateCache() = 0;
};

} // namespace Graffodil
