#include <graffodil/PanZoomTool.h>
#include <graffodil/GraphScene.h>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QScrollBar>

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
    if (auto *v = activeView())
        v->viewport()->setCursor(Qt::OpenHandCursor);
    emit statusMessage(tr("Left-drag to pan, middle-drag to zoom"));
}

void PanZoomTool::deactivate()
{
    m_isPanning = false;
    m_isZooming = false;
    if (auto *v = activeView())
        v->viewport()->setCursor(Qt::ArrowCursor);
    GraphTool::deactivate();
}

void PanZoomTool::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isPanning = true;
        m_lastScreenPos = event->screenPos();
        if (auto *v = activeView())
            v->viewport()->setCursor(Qt::ClosedHandCursor);
    } else if (event->button() == Qt::MiddleButton) {
        m_isZooming = true;
        m_lastScreenPos = event->screenPos();
        m_zoomAnchorScene = event->scenePos();
        if (auto *v = activeView())
            v->viewport()->setCursor(Qt::SizeVerCursor);
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
    } else if (m_isZooming) {
        QPointF delta = event->screenPos() - m_lastScreenPos;
        m_lastScreenPos = event->screenPos();

        double dy = -delta.y();
        double factor = 1.0 + dy * 0.005;
        factor = qBound(0.5, factor, 2.0);

        v->setTransformationAnchor(QGraphicsView::NoAnchor);
        QPointF viewAnchor = v->mapFromScene(m_zoomAnchorScene);
        v->scale(factor, factor);
        QPointF newViewAnchor = v->mapFromScene(m_zoomAnchorScene);
        QPointF drift = newViewAnchor - viewAnchor;
        v->horizontalScrollBar()->setValue(
            v->horizontalScrollBar()->value() + static_cast<int>(drift.x()));
        v->verticalScrollBar()->setValue(
            v->verticalScrollBar()->value() + static_cast<int>(drift.y()));
        v->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    }
}

void PanZoomTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isPanning) {
        m_isPanning = false;
        if (auto *v = activeView())
            v->viewport()->setCursor(Qt::OpenHandCursor);
    } else if (event->button() == Qt::MiddleButton && m_isZooming) {
        m_isZooming = false;
        if (auto *v = activeView())
            v->viewport()->setCursor(Qt::OpenHandCursor);
    }
}

} // namespace Graffodil
