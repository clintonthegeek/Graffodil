#pragma once

#include <QGraphicsItem>
#include <QString>

namespace Graffodil {

class IGraphNode;

/// Anything that can participate as an edge in a GraphScene.
/// Consumers can use the provided GraphEdgeItem or implement their own.
class IGraphEdge {
public:
    virtual ~IGraphEdge() = default;

    virtual QString edgeId() const = 0;
    virtual IGraphNode *sourceNode() const = 0;
    virtual IGraphNode *targetNode() const = 0;

    /// Bridge to QGraphicsItem, same pattern as IGraphNode.
    virtual QGraphicsItem *graphicsItem() = 0;
};

} // namespace Graffodil
