#pragma once

#include <QGraphicsItem>
#include <QVector>
#include <QPointF>
#include <QColor>

namespace Graffodil {

class BatchNodeItem : public QGraphicsItem {
public:
    struct NodeData {
        QPointF position;
        double radius = 5.0;
        QColor color = QColor(100, 150, 200);
    };

    BatchNodeItem(QGraphicsItem *parent = nullptr);

    void setNodes(const QVector<NodeData> &nodes);
    void updatePositions(const QVector<QPointF> &positions);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QVector<NodeData> m_nodes;
};

} // namespace Graffodil
