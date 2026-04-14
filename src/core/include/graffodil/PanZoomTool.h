#pragma once

#include <graffodil/GraphTool.h>
#include <QPointF>

class QGraphicsView;

namespace Graffodil {

/// Pan + zoom viewport manipulation.
///
/// Default bindings (when used as a standalone tool):
/// - Pan: left-drag (change with setPanButton)
/// - Wheel zoom: Ctrl+wheel (change with setZoomWheelModifier)
/// - Click-drag zoom: disabled by default (set a button with setZoomButton)
///
/// Designed to be used either as a standalone tool or as a sub-tool inside
/// a CompositeTool. The default bindings in CompositeTool's DefaultGraphTool
/// change the pan button to Middle so it composes with SelectMoveTool on Left.
class PanZoomTool : public GraphTool {
    Q_OBJECT
public:
    using GraphTool::GraphTool;

    void activate(GraphScene *scene) override;
    void deactivate() override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;

    /// Which button starts a pan drag. Default Qt::LeftButton.
    /// Set Qt::NoButton to disable drag-to-pan.
    void setPanButton(Qt::MouseButton button);
    Qt::MouseButton panButton() const { return m_panButton; }

    /// Which button starts a vertical-drag zoom. Default Qt::MiddleButton.
    /// Set Qt::NoButton to disable drag-to-zoom.
    void setZoomButton(Qt::MouseButton button);
    Qt::MouseButton zoomButton() const { return m_zoomButton; }

    /// Modifier required for wheel zoom. Default Qt::ControlModifier.
    /// Set Qt::NoModifier to zoom on any wheel event. Set to an
    /// unreachable value (e.g. Qt::GroupSwitchModifier) to disable wheel zoom.
    void setZoomWheelModifier(Qt::KeyboardModifiers mod);
    Qt::KeyboardModifiers zoomWheelModifier() const { return m_wheelZoomMod; }

private:
    QGraphicsView *activeView() const;
    void applyZoom(qreal factor, const QPointF &anchorScenePos);

    Qt::MouseButton m_panButton = Qt::LeftButton;
    Qt::MouseButton m_zoomButton = Qt::MiddleButton;
    Qt::KeyboardModifiers m_wheelZoomMod = Qt::ControlModifier;

    bool m_isPanning = false;
    bool m_isZooming = false;
    QPointF m_lastScreenPos;
    QPointF m_zoomAnchorScene;
};

} // namespace Graffodil
