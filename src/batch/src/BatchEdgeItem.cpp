#include "BatchEdgeItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <algorithm>

namespace Graffodil {

BatchEdgeItem::BatchEdgeItem(QGraphicsItem *parent) : QGraphicsItem(parent)
{
    setZValue(0);
}

void BatchEdgeItem::setEdges(const QVector<EdgeData> &edges)
{
    prepareGeometryChange();
    m_edges = edges;
    m_normalCount = static_cast<int>(
        std::partition(m_edges.begin(), m_edges.end(),
                       [](const EdgeData &e) { return !e.dimmed; })
        - m_edges.begin());
    update();
}

void BatchEdgeItem::updateLines(const QVector<QLineF> &lines)
{
    int count = qMin(lines.size(), m_edges.size());
    for (int i = 0; i < count; ++i)
        m_edges[i].line = lines[i];
    update();
}

QRectF BatchEdgeItem::boundingRect() const
{
    return QRectF(-10000, -10000, 20000, 20000);
}

void BatchEdgeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (m_edges.isEmpty()) return;

    double lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform());
    if (lod < 0.03) return;

    // Draw normal edges
    if (m_normalCount > 0) {
        QPen normalPen(QColor(150, 150, 150, 60), 0.8);
        normalPen.setCosmetic(true);
        painter->setPen(normalPen);
        QVector<QLineF> lines;
        lines.reserve(m_normalCount);
        for (int i = 0; i < m_normalCount; ++i)
            lines.append(m_edges[i].line);
        painter->drawLines(lines);
    }

    // Draw dimmed edges
    int dimmedCount = m_edges.size() - m_normalCount;
    if (dimmedCount > 0) {
        QPen dimmedPen(QColor(200, 200, 200, 20), 0.3);
        dimmedPen.setCosmetic(true);
        painter->setPen(dimmedPen);
        QVector<QLineF> lines;
        lines.reserve(dimmedCount);
        for (int i = m_normalCount; i < m_edges.size(); ++i)
            lines.append(m_edges[i].line);
        painter->drawLines(lines);
    }
}

} // namespace Graffodil
