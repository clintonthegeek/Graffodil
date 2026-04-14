#pragma once

#include <QObject>

class QGraphicsSceneMouseEvent;
class QGraphicsSceneWheelEvent;
class QKeyEvent;

namespace Graffodil {

class GraphScene;

class GraphTool : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

    virtual void activate(GraphScene *scene);
    virtual void deactivate();

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) = 0;
    virtual void wheelEvent(QGraphicsSceneWheelEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

    GraphScene *scene() const;

signals:
    void statusMessage(const QString &msg);

protected:
    GraphScene *m_scene = nullptr;
};

} // namespace Graffodil
