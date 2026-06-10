#pragma once

#include <graffodil/IGraphNode.h>
#include <QGraphicsItem>
#include <QList>
#include <QPointF>
#include <QString>

namespace Graffodil {

/// Anything that can participate as an edge in a GraphScene.
/// Consumers can use the provided GraphEdgeItem or implement their own.
class IGraphEdge {
public:
    virtual ~IGraphEdge() = default;

    virtual QString edgeId() const = 0;
    virtual IGraphNode *sourceNode() const = 0;
    virtual IGraphNode *targetNode() const = 0;

    /// Fast-path endpoint ids, implemented in terms of sourceNode()/targetNode().
    /// Returns an empty string if the endpoint is null.
    QString sourceNodeId() const {
        IGraphNode *n = sourceNode();
        return n ? n->nodeId() : QString();
    }
    QString targetNodeId() const {
        IGraphNode *n = targetNode();
        return n ? n->nodeId() : QString();
    }

    /// Notification that either endpoint node may have moved or changed
    /// geometry. The scene calls this from adjustEdgesForNode(),
    /// adjustAllEdges(), and applyLayout(). Implementations should recompute
    /// their visual path. Default is a no-op so data-only edges stay valid.
    virtual void endpointsChanged() {}

    /// Routing hints delivered by GraphScene::applyLayout() when a layout
    /// engine produced LayoutResult::edgeWaypoints for this edge's
    /// (sourceId, targetId) pair. Scene coordinates, ordered source→target.
    /// Default ignores them; GraphEdgeItem feeds them to its path strategy.
    virtual void setWaypoints(const QList<QPointF> &waypoints)
    {
        Q_UNUSED(waypoints);
    }

    /// Bridge to QGraphicsItem, same pattern as IGraphNode.
    virtual QGraphicsItem *graphicsItem() = 0;
};

} // namespace Graffodil
