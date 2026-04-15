#pragma once

#include <QGraphicsItem>
#include <QVector>
#include <QPointF>
#include <QColor>
#include <QPixmap>
#include <QSizeF>

namespace Graffodil {

class BatchNodeItem : public QGraphicsItem {
public:
    struct NodeData {
        QPointF position;
        double radius = 5.0;
        QColor color = QColor(100, 150, 200);
        QPixmap cachedPixmap;   // empty if the node has no IRenderableCache
        QSizeF cachedSize;      // logical size to draw cachedPixmap at
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
