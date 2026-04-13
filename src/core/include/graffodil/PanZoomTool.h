#pragma once

#include <graffodil/GraphTool.h>
#include <QPointF>

class QGraphicsView;

namespace Graffodil {

class PanZoomTool : public GraphTool {
    Q_OBJECT
public:
    using GraphTool::GraphTool;

    void activate(GraphScene *scene) override;
    void deactivate() override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QGraphicsView *activeView() const;

    bool m_isPanning = false;
    bool m_isZooming = false;
    QPointF m_lastScreenPos;
    QPointF m_zoomAnchorScene;
};

} // namespace Graffodil
