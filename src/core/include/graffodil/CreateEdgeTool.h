#pragma once

#include <graffodil/GraphTool.h>
#include <graffodil/Types.h>
#include <QPointF>

class QGraphicsLineItem;

namespace Graffodil {

class IGraphNode;
class AnchorHighlight;

/// Edge creation tool. Supports two interaction styles:
///
/// **Click-click:** click source node → preview line follows cursor →
/// click target node → edge created.
///
/// **Drag:** press on source node → drag → release on target node → edge
/// created. Short drags (below threshold) fall through into click-click
/// mode so you can mix and match.
///
/// While an edge creation is in progress, hovering near a target node
/// snaps the preview line to the nearest anchor on that node and
/// highlights it.
class CreateEdgeTool : public GraphTool {
    Q_OBJECT
public:
    explicit CreateEdgeTool(QObject *parent = nullptr);
    ~CreateEdgeTool() override;

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
    void updatePreview(const QPointF &cursorScenePos);
    void commitEdge(IGraphNode *target, const QPointF &cursorScenePos);
    void cancel();

    IGraphNode *m_sourceNode = nullptr;
    Side m_sourceSide = Side::Right;
    QPointF m_pressScenePos;
    bool m_sawDrag = false;
    QGraphicsLineItem *m_previewLine = nullptr;
    AnchorHighlight *m_highlight = nullptr;
};

} // namespace Graffodil
