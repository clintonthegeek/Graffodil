#pragma once

#include <graffodil/GraphTool.h>
#include <graffodil/Types.h>
#include <QPointF>
#include <QPointer>
#include <QString>

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
/// While creation is in progress, hovering a target node snaps the
/// preview line to the nearest anchor on that node and highlights it.
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

    /// When true, release without drag cancels instead of entering
    /// click-click mode. Matches the interaction style of node editors
    /// (Blender, Unreal Blueprint, etc.) and suits anchor-initiated
    /// edge creation embedded in a broader default tool. Default: false.
    void setDragOnly(bool dragOnly);
    bool isDragOnly() const { return m_dragOnly; }

    /// How far outside a node's rect the hover-preview still counts as
    /// "over that node" for anchor highlighting. Matches the radius used
    /// by CompositeTool::addAnchorRoute so that the pointing-finger
    /// cursor and the anchor dots appear together. Default 12.0.
    void setAnchorHoverRadius(qreal radius) { m_anchorHoverRadius = radius; }
    qreal anchorHoverRadius() const { return m_anchorHoverRadius; }

signals:
    void edgeRequested(IGraphNode *source, const QString &sourceAnchorId,
                       IGraphNode *target, const QString &targetAnchorId);

private:
    IGraphNode *findNodeAt(const QPointF &scenePos) const;
    Anchor nearestAnchor(IGraphNode *node, const QPointF &scenePos) const;
    void updatePreview(const QPointF &cursorScenePos);
    void commitEdge(IGraphNode *target, const QPointF &cursorScenePos);
    void cancel();

    IGraphNode *m_sourceNode = nullptr;
    QString m_sourceAnchorId;
    QPointF m_pressScenePos;
    bool m_sawDrag = false;
    bool m_dragOnly = false;
    qreal m_anchorHoverRadius = 12.0;
    QGraphicsLineItem *m_previewLine = nullptr;
    // QPointer so we notice when the scene deletes the item during its own
    // destruction — otherwise ~CreateEdgeTool would dereference a dangling
    // pointer if the scene (a sibling QObject) is destroyed first.
    QPointer<AnchorHighlight> m_highlight;
};

} // namespace Graffodil
