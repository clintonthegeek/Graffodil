#pragma once

#include <graffodil/IGraphNode.h>
#include <QGraphicsItem>
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

    /// Bridge to QGraphicsItem, same pattern as IGraphNode.
    virtual QGraphicsItem *graphicsItem() = 0;
};

} // namespace Graffodil
