#pragma once

#include <QPointF>
#include <QList>
#include <QLineF>

namespace Graffodil {

class IGraphNode;

/// Pluggable alignment-guide strategy used by SelectMoveTool during node
/// drags.
///
/// Consumers implement this to get pixel-snap behaviour (align to other
/// nodes, align to a grid, Canvas-style ruler guides, etc.). Core ships
/// no default implementation — with no strategy installed, SelectMoveTool
/// behaves exactly as before (free-drag, no snapping, no guides).
///
/// The strategy is consulted per mouseMove during an active drag. It
/// receives the *proposed* scene position of the primary drag node
/// (`m_dragNode` — the node under the cursor at press time) and may
/// return an adjusted position plus zero or more guide lines for the
/// tool to paint as overlays.
class IAlignmentStrategy {
public:
    virtual ~IAlignmentStrategy() = default;

    struct Result {
        /// Adjusted position for the primary drag node. Return the input
        /// unchanged to decline snapping on this frame.
        QPointF position;
        /// Guide lines to paint (scene coordinates). Empty = no overlay.
        QList<QLineF> guides;
    };

    /// Called each mouseMove during an active drag.
    /// `proposed` is the free-drag position the tool would apply with no
    /// strategy installed. `primary` is the node under the cursor; all
    /// other selected nodes will be translated by the same delta that
    /// the strategy applies to `primary`.
    /// `allNodes` is every node currently in the scene — the strategy
    /// can pick alignment targets from it.
    virtual Result align(IGraphNode *primary,
                         const QPointF &proposed,
                         const QList<IGraphNode *> &allNodes) = 0;
};

} // namespace Graffodil
