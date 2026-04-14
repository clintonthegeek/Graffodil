#pragma once

#include <graffodil/IGraphEdge.h>
#include <graffodil/Types.h>
#include <QGraphicsPathItem>
#include <QList>
#include <QPointF>
#include <QString>
#include <memory>

namespace Graffodil {

class IGraphNode;
class EdgePathStrategy;
class TerminusStyle;

/// A concrete edge item connecting two nodes at specified anchors.
///
/// Anchors are referenced by string id — whatever the node's anchors()
/// returns. Pass an empty string as the anchor id on either end to make
/// that end "swivel": it resolves dynamically via
/// IGraphNode::anchorToward() each time the edge adjusts.
class GraphEdgeItem : public QGraphicsPathItem, public IGraphEdge {
public:
    GraphEdgeItem(IGraphNode *source, const QString &sourceAnchorId,
                  IGraphNode *target, const QString &targetAnchorId,
                  EdgePathStrategy *strategy);
    ~GraphEdgeItem() override;

    // --- IGraphEdge ---
    QString edgeId() const override;
    IGraphNode *sourceNode() const override;
    IGraphNode *targetNode() const override;
    QGraphicsItem *graphicsItem() override;

    // --- Configuration ---
    void setPathStrategy(EdgePathStrategy *strategy);
    void setTerminus(ArrowEnd end, std::unique_ptr<TerminusStyle> style);
    void setPen(const QPen &pen);
    void setHighlighted(bool highlighted);
    void setWaypoints(const QList<QPointF> &waypoints);

    /// Recompute path from current node anchor positions.
    void adjust();

    QString sourceAnchorId() const { return m_sourceAnchorId; }
    QString targetAnchorId() const { return m_targetAnchorId; }
    void setSourceAnchorId(const QString &id) { m_sourceAnchorId = id; }
    void setTargetAnchorId(const QString &id) { m_targetAnchorId = id; }

    /// Swap source and target (and their anchor ids).
    /// Terminus styles stay on their respective ends so the arrow direction
    /// flips naturally.
    void reverse();

    /// Width (in scene units) of the clickable area centered on the path.
    /// Independent of the visual pen width. Default 8.0.
    void setHitWidth(qreal width);
    qreal hitWidth() const { return m_hitWidth; }

    QPainterPath shape() const override;
    QRectF boundingRect() const override;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

private:
    /// Resolve the current anchor from the node (either by id or dynamically).
    Anchor resolveSourceAnchor() const;
    Anchor resolveTargetAnchor() const;

    QString m_id;
    IGraphNode *m_source;
    IGraphNode *m_target;
    QString m_sourceAnchorId;
    QString m_targetAnchorId;
    std::unique_ptr<EdgePathStrategy> m_strategy;
    std::unique_ptr<TerminusStyle> m_sourceTerminus;
    std::unique_ptr<TerminusStyle> m_targetTerminus;
    QList<QPointF> m_waypoints;
    qreal m_hitWidth = 8.0;
    bool m_highlighted = false;
};

} // namespace Graffodil
