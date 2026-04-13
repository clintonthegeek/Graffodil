#pragma once

#include <graffodil/GraphTool.h>
#include <graffodil/Types.h>
#include <QList>
#include <QPointF>

namespace Graffodil {

class IGraphNode;
class IGraphEdge;

class SelectMoveTool : public GraphTool {
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
    void deleteRequested(const QList<IGraphNode *> &nodes,
                         const QList<IGraphEdge *> &edges);
    void nodeMoved(IGraphNode *node, const QPointF &oldPos, const QPointF &newPos);

private:
    IGraphNode *m_dragNode = nullptr;
    QPointF m_dragStartPos;
    bool m_dragging = false;
};

} // namespace Graffodil
