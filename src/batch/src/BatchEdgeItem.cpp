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
    recomputeBounds();
    update();
}

void BatchEdgeItem::updateLines(const QVector<QLineF> &lines)
{
    prepareGeometryChange();
    int count = qMin(lines.size(), m_edges.size());
    for (int i = 0; i < count; ++i)
        m_edges[i].line = lines[i];
    recomputeBounds();
    update();
}

QRectF BatchEdgeItem::boundingRect() const
{
    return m_bounds;
}

void BatchEdgeItem::setEdgeColors(const QColor &normal, const QColor &dimmed)
{
    if (m_normalColor == normal && m_dimmedColor == dimmed)
        return;
    m_normalColor = normal;
    m_dimmedColor = dimmed;
    update();
}

void BatchEdgeItem::recomputeBounds()
{
    QRectF b;
    for (const auto &e : m_edges)
        b |= QRectF(e.line.p1(), e.line.p2()).normalized();
    m_bounds = b.isNull() ? b : b.adjusted(-1.0, -1.0, 1.0, 1.0);
}

void BatchEdgeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (m_edges.isEmpty()) return;

    double lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform());
    if (lod < 0.03) return;

    // Draw normal edges
    if (m_normalCount > 0) {
        QPen normalPen(m_normalColor, 0.8);
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
        QPen dimmedPen(m_dimmedColor, 0.3);
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
