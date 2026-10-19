#include <graffodil/GraphMinimap.h>

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPainter>
#include <QMouseEvent>

namespace Graffodil {

static constexpr int kMinimapWidth = 150;
static constexpr int kMinimapHeight = 100;
static constexpr int kMinimapMargin = 8;

GraphMinimap::GraphMinimap(QGraphicsView *graphicsView, QWidget *parent)
    : QWidget(parent)
    , m_graphicsView(graphicsView)
{
    setFixedSize(kMinimapWidth, kMinimapHeight);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);

    if (m_graphicsView) {
        m_graphicsView->installEventFilter(this);
        setParent(m_graphicsView);
    }
}

void GraphMinimap::setEnabled(bool visible)
{
    setVisible(visible);
}

QRectF GraphMinimap::effectiveSceneRect(QGraphicsView *view)
{
    if (!view || !view->scene())
        return {};
    QRectF rect = view->scene()->sceneRect();
    if (rect.isEmpty()) {
        rect = view->scene()->itemsBoundingRect();
        if (rect.isNull())
            return {};
        rect.adjust(-50, -50, 50, 50);
    }
    return rect.united(
        view->mapToScene(view->viewport()->rect()).boundingRect());
}

void GraphMinimap::paintEvent(QPaintEvent * /*event*/)
{
    if (!m_graphicsView || !m_graphicsView->scene())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(rect(), QColor(30, 30, 30, 200));
    painter.setPen(QPen(QColor(100, 100, 100), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    // Scene bounds
    QRectF sceneRect = effectiveSceneRect(m_graphicsView);
    if (sceneRect.isNull())
        return;
    // A zero-extent scene rect would scale by infinity (SIGFPE on some
    // platforms, garbage painting on others).
    if (sceneRect.width() <= 0.0 || sceneRect.height() <= 0.0)
        return;

    // Scale scene to fit minimap
    qreal scaleX = (width() - 4) / sceneRect.width();
    qreal scaleY = (height() - 4) / sceneRect.height();
    qreal scale = qMin(scaleX, scaleY);

    painter.save();
    painter.translate(2, 2);
    painter.scale(scale, scale);
    painter.translate(-sceneRect.topLeft());

    // Render scene items
    m_graphicsView->scene()->render(&painter, QRectF(), sceneRect);
    painter.restore();

    // Viewport rectangle
    QRectF viewRect = m_graphicsView->mapToScene(
        m_graphicsView->viewport()->rect()).boundingRect();
    QRectF mappedView(
        2 + (viewRect.x() - sceneRect.x()) * scale,
        2 + (viewRect.y() - sceneRect.y()) * scale,
        viewRect.width() * scale,
        viewRect.height() * scale
    );

    painter.setPen(QPen(QColor(0, 150, 255), 2));
    painter.setBrush(QColor(0, 150, 255, 30));
    painter.drawRect(mappedView);
}

void GraphMinimap::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        centerViewOn(mapToScene(event->pos()));
        event->accept();
    }
}

void GraphMinimap::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        centerViewOn(mapToScene(event->pos()));
        event->accept();
    }
}

void GraphMinimap::mouseReleaseEvent(QMouseEvent *event)
{
    // Without this the drag latched on for the widget's lifetime: every later
    // hover re-centred the view with no button held.
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}

bool GraphMinimap::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_graphicsView) {
        if (event->type() == QEvent::Resize) {
            QSize viewSize = m_graphicsView->size();
            move(viewSize.width() - width() - kMinimapMargin,
                 viewSize.height() - height() - kMinimapMargin);
        }
        if (event->type() == QEvent::Resize || event->type() == QEvent::Paint)
            update();
    }
    return QWidget::eventFilter(watched, event);
}

QPointF GraphMinimap::mapToScene(const QPoint &minimapPos) const
{
    if (!m_graphicsView || !m_graphicsView->scene())
        return {};

    QRectF sceneRect = effectiveSceneRect(m_graphicsView);
    if (sceneRect.isNull() || sceneRect.width() <= 0.0 || sceneRect.height() <= 0.0)
        return {};

    qreal scaleX = (width() - 4) / sceneRect.width();
    qreal scaleY = (height() - 4) / sceneRect.height();
    qreal scale = qMin(scaleX, scaleY);

    return {
        sceneRect.x() + (minimapPos.x() - 2) / scale,
        sceneRect.y() + (minimapPos.y() - 2) / scale
    };
}

void GraphMinimap::centerViewOn(const QPointF &scenePos)
{
    if (m_graphicsView) {
        m_graphicsView->centerOn(scenePos);
        update();
    }
}

} // namespace Graffodil
