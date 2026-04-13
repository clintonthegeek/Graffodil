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
    bool m_highlighted = false;
};

} // namespace Graffodil
