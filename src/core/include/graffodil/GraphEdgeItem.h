#pragma once

#include <graffodil/IGraphEdge.h>
#include <graffodil/Types.h>
#include <QGraphicsPathItem>
#include <memory>

namespace Graffodil {

class IGraphNode;
class EdgePathStrategy;
class TerminusStyle;

class GraphEdgeItem : public QGraphicsPathItem, public IGraphEdge {
public:
    GraphEdgeItem(IGraphNode *source, Side sourceSide,
                  IGraphNode *target, Side targetSide,
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

    // --- Geometry update ---
    void adjust();

    Side sourceSide() const { return m_sourceSide; }
    Side targetSide() const { return m_targetSide; }

    /// Swap source and target (and their sides).
    /// Terminus styles stay on their respective ends (source-end stays on
    /// what is now the source, etc.) so the arrow direction flips naturally.
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
    QString m_id;
    IGraphNode *m_source;
    IGraphNode *m_target;
    Side m_sourceSide;
    Side m_targetSide;
    std::unique_ptr<EdgePathStrategy> m_strategy;
    std::unique_ptr<TerminusStyle> m_sourceTerminus;
    std::unique_ptr<TerminusStyle> m_targetTerminus;
    QList<QPointF> m_waypoints;
    qreal m_hitWidth = 8.0;
    bool m_highlighted = false;
};

} // namespace Graffodil
