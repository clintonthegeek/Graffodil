#pragma once

#include <QWidget>

class QGraphicsView;

namespace Graffodil {

class GraphMinimap : public QWidget {
    Q_OBJECT
public:
    explicit GraphMinimap(QGraphicsView *graphicsView, QWidget *parent = nullptr);

    void setEnabled(bool visible);

    static QRectF effectiveSceneRect(QGraphicsView *view);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QPointF mapToScene(const QPoint &minimapPos) const;
    void centerViewOn(const QPointF &scenePos);

    QGraphicsView *m_graphicsView;
    bool m_dragging = false;
};

} // namespace Graffodil
