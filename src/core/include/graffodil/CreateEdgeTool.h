#pragma once

#include <graffodil/GraphTool.h>
#include <graffodil/Types.h>

class QGraphicsLineItem;

namespace Graffodil {

class IGraphNode;
class AnchorHighlight;

class CreateEdgeTool : public GraphTool {
    Q_OBJECT
public:
    using GraphTool::GraphTool;

    void activate(GraphScene *scene) override;
    void deactivate() override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

signals:
    void edgeRequested(IGraphNode *source, Side sourceSide,
                       IGraphNode *target, Side targetSide);

private:
    IGraphNode *findNodeAt(const QPointF &scenePos) const;
    Side nearestSide(IGraphNode *node, const QPointF &scenePos) const;
    void cancel();

    IGraphNode *m_sourceNode = nullptr;
    Side m_sourceSide = Side::Right;
    QGraphicsLineItem *m_previewLine = nullptr;
    AnchorHighlight *m_highlight = nullptr;
};

} // namespace Graffodil
