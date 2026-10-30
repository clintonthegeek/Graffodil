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

    /// Theme the two batch pens. Defaults to the historical hard-coded
    /// values; BatchRenderer keeps them in sync with the scene's Palette
    /// on each synchronize().
    void setEdgeColors(const QColor &normal, const QColor &dimmed);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QVector<EdgeData> m_edges;
    int m_normalCount = 0;
    QColor m_normalColor = QColor(150, 150, 150, 60);
    QColor m_dimmedColor = QColor(200, 200, 200, 20);
    QRectF m_bounds;
    void recomputeBounds();
};

} // namespace Graffodil
