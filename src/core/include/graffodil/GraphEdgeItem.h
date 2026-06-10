#pragma once

#include <graffodil/IGraphEdge.h>
#include <graffodil/Types.h>
#include <QColor>
#include <QGraphicsPathItem>
#include <QList>
#include <QPointF>
#include <QString>
#include <memory>
#include <optional>

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
                  std::unique_ptr<EdgePathStrategy> strategy);

    [[deprecated("pass std::unique_ptr<EdgePathStrategy>; ownership transfers either way")]]
    GraphEdgeItem(IGraphNode *source, const QString &sourceAnchorId,
                  IGraphNode *target, const QString &targetAnchorId,
                  EdgePathStrategy *strategy);

    ~GraphEdgeItem() override;

    // --- IGraphEdge ---
    QString edgeId() const override;
    IGraphNode *sourceNode() const override;
    IGraphNode *targetNode() const override;
    QGraphicsItem *graphicsItem() override;
    void endpointsChanged() override { adjust(); }

    // --- Configuration ---
    void setPathStrategy(std::unique_ptr<EdgePathStrategy> strategy);
    [[deprecated("pass std::unique_ptr<EdgePathStrategy>; ownership transfers either way")]]
    void setPathStrategy(EdgePathStrategy *strategy);
    void setTerminus(ArrowEnd end, std::unique_ptr<TerminusStyle> style);
    void setPen(const QPen &pen);
    void setHighlighted(bool highlighted);
    void setWaypoints(const QList<QPointF> &waypoints) override;

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

    /// Stroke the edge with a linear gradient running from the source endpoint
    /// to the target endpoint. Useful for indicating directionality without
    /// relying solely on an arrowhead. Clear with clearGradient().
    /// The gradient is a straight-line approximation between endpoints; on
    /// strongly curved paths the color will not track the curve.
    void setGradient(const QColor &fromColor, const QColor &toColor);
    void clearGradient();
    bool hasGradient() const { return m_gradient.has_value(); }

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
    std::optional<std::pair<QColor, QColor>> m_gradient;
};

} // namespace Graffodil
