#pragma once

#include <QGraphicsItem>
#include <QVector>
#include <QLineF>

namespace Graffodil {

class BatchEdgeItem : public QGraphicsItem {
public:
    struct EdgeData {
        QLineF line;
        bool dimmed = false;
    };

    BatchEdgeItem(QGraphicsItem *parent = nullptr);

    void setEdges(const QVector<EdgeData> &edges);
    void updateLines(const QVector<QLineF> &lines);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QVector<EdgeData> m_edges;
    int m_normalCount = 0;
    QRectF m_bounds;
    void recomputeBounds();
};

} // namespace Graffodil
