#include <graffodil/PanZoomTool.h>
#include <graffodil/GraphScene.h>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QScrollBar>
#include <cmath>

namespace Graffodil {

QGraphicsView *PanZoomTool::activeView() const
{
    if (!m_scene || m_scene->views().isEmpty())
        return nullptr;
    return m_scene->views().first();
}

void PanZoomTool::activate(GraphScene *scene)
{
    GraphTool::activate(scene);
    emit statusMessage(tr("Drag to pan, scroll wheel to zoom"));
}

void PanZoomTool::deactivate()
{
    m_isPanning = false;
    m_isZooming = false;
    if (auto *v = activeView())
        v->viewport()->unsetCursor();
    GraphTool::deactivate();
}

void PanZoomTool::setPanButton(Qt::MouseButton button) { m_panButton = button; }
void PanZoomTool::setZoomButton(Qt::MouseButton button) { m_zoomButton = button; }
void PanZoomTool::setZoomWheelModifier(Qt::KeyboardModifiers mod) { m_wheelZoomMod = mod; }

void PanZoomTool::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_panButton != Qt::NoButton && event->button() == m_panButton) {
        m_isPanning = true;
        m_lastScreenPos = event->screenPos();
        if (auto *v = activeView())
            v->viewport()->setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else if (m_zoomButton != Qt::NoButton && event->button() == m_zoomButton) {
        m_isZooming = true;
        m_lastScreenPos = event->screenPos();
        m_zoomAnchorScene = event->scenePos();
        if (auto *v = activeView())
            v->viewport()->setCursor(Qt::SizeVerCursor);
        event->accept();
    }
}

void PanZoomTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    auto *v = activeView();
    if (!v) return;

    if (m_isPanning) {
        QPointF delta = event->screenPos() - m_lastScreenPos;
        m_lastScreenPos = event->screenPos();
        v->horizontalScrollBar()->setValue(
            v->horizontalScrollBar()->value() - static_cast<int>(delta.x()));
        v->verticalScrollBar()->setValue(
            v->verticalScrollBar()->value() - static_cast<int>(delta.y()));
        event->accept();
    } else if (m_isZooming) {
        QPointF delta = event->screenPos() - m_lastScreenPos;
        m_lastScreenPos = event->screenPos();

        double dy = -delta.y();
        double factor = 1.0 + dy * 0.005;
        factor = qBound(0.5, factor, 2.0);
        applyZoom(factor, m_zoomAnchorScene);
        event->accept();
    }
}

void PanZoomTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == m_panButton && m_isPanning) {
        m_isPanning = false;
        if (auto *v = activeView())
            v->viewport()->unsetCursor();
        event->accept();
    } else if (event->button() == m_zoomButton && m_isZooming) {
        m_isZooming = false;
        if (auto *v = activeView())
            v->viewport()->unsetCursor();
        event->accept();
    }
}

void PanZoomTool::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    // Only zoom when the required modifier is present (NoModifier means always).
    if (m_wheelZoomMod != Qt::NoModifier
        && (event->modifiers() & m_wheelZoomMod) != m_wheelZoomMod)
        return;

    auto *v = activeView();
    if (!v) return;

    const double steps = event->delta() / 120.0;
    const double factor = std::pow(1.15, steps);

    // For wheel events the cursor is over the view, so let Qt's built-in
    // AnchorUnderMouse handle the re-centering. It's more reliable than
    // scrollbar manipulation (which can fail when bars are hidden).
    v->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    v->scale(factor, factor);

    event->accept();
}

void PanZoomTool::applyZoom(qreal factor, const QPointF &anchorScenePos)
{
    auto *v = activeView();
    if (!v) return;

    // Zoom anchored at the scene point under the cursor — after scaling,
    // adjust scroll so the same scene point remains under the cursor.
    v->setTransformationAnchor(QGraphicsView::NoAnchor);
    QPointF viewAnchor = v->mapFromScene(anchorScenePos);
    v->scale(factor, factor);
    QPointF newViewAnchor = v->mapFromScene(anchorScenePos);
    QPointF drift = newViewAnchor - viewAnchor;
    v->horizontalScrollBar()->setValue(
        v->horizontalScrollBar()->value() + static_cast<int>(drift.x()));
    v->verticalScrollBar()->setValue(
        v->verticalScrollBar()->value() + static_cast<int>(drift.y()));
    v->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
}

} // namespace Graffodil
