#include <graffodil/TerminusStyle.h>
#include <QPolygonF>
#include <cmath>

namespace Graffodil {

static constexpr qreal kArrowSize = 10.0;
static constexpr qreal kArrowAngle = M_PI / 6.0; // 30 degrees

static double directionAngle(const QPointF &tip, const QPointF &from)
{
    return std::atan2(-(tip.y() - from.y()), tip.x() - from.x());
}

static QPointF insetPoint(const QPointF &tip, double angle, double depth)
{
    return tip - QPointF(std::cos(angle) * depth, -std::sin(angle) * depth);
}

// --- TriangleTerminus ---

QPointF TriangleTerminus::draw(QPainter *painter, const QPointF &tip,
                                const QPointF &from, const QPen &edgePen)
{
    const double angle = directionAngle(tip, from);
    const QPointF p1 = tip - QPointF(std::cos(angle - kArrowAngle) * kArrowSize,
                                     -std::sin(angle - kArrowAngle) * kArrowSize);
    const QPointF p2 = tip - QPointF(std::cos(angle + kArrowAngle) * kArrowSize,
                                     -std::sin(angle + kArrowAngle) * kArrowSize);

    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(edgePen.color());
    painter->drawPolygon(QPolygonF{tip, p1, p2});
    painter->restore();

    return insetPoint(tip, angle, kArrowSize * 0.8);
}

qreal TriangleTerminus::size() const { return kArrowSize; }

// --- OpenArrowTerminus ---

QPointF OpenArrowTerminus::draw(QPainter *painter, const QPointF &tip,
                                 const QPointF &from, const QPen &edgePen)
{
    const double angle = directionAngle(tip, from);
    const QPointF p1 = tip - QPointF(std::cos(angle - kArrowAngle) * kArrowSize,
                                     -std::sin(angle - kArrowAngle) * kArrowSize);
    const QPointF p2 = tip - QPointF(std::cos(angle + kArrowAngle) * kArrowSize,
                                     -std::sin(angle + kArrowAngle) * kArrowSize);

    painter->save();
    painter->setPen(QPen(edgePen.color(), edgePen.widthF(), Qt::SolidLine,
                         Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);
    painter->drawPolyline(QPolygonF{p1, tip, p2});
    painter->restore();

    return tip;
}

qreal OpenArrowTerminus::size() const { return kArrowSize; }

// --- DiamondTerminus ---

QPointF DiamondTerminus::draw(QPainter *painter, const QPointF &tip,
                               const QPointF &from, const QPen &edgePen)
{
    const double angle = directionAngle(tip, from);
    const double halfSize = kArrowSize * 0.6;
    const QPointF mid = insetPoint(tip, angle, halfSize);
    const QPointF back = insetPoint(tip, angle, halfSize * 2);

    const double perpAngle = angle + M_PI / 2.0;
    const QPointF side1 = mid + QPointF(std::cos(perpAngle) * halfSize * 0.5,
                                        -std::sin(perpAngle) * halfSize * 0.5);
    const QPointF side2 = mid - QPointF(std::cos(perpAngle) * halfSize * 0.5,
                                        -std::sin(perpAngle) * halfSize * 0.5);

    painter->save();
    painter->setPen(QPen(edgePen.color(), edgePen.widthF()));
    painter->setBrush(m_filled ? edgePen.color() : Qt::NoBrush);
    painter->drawPolygon(QPolygonF{tip, side1, back, side2});
    painter->restore();

    return back;
}

qreal DiamondTerminus::size() const { return kArrowSize * 1.2; }

// --- CircleTerminus ---

QPointF CircleTerminus::draw(QPainter *painter, const QPointF &tip,
                              const QPointF &from, const QPen &edgePen)
{
    const double angle = directionAngle(tip, from);
    const double radius = kArrowSize * 0.4;
    const QPointF center = insetPoint(tip, angle, radius);

    painter->save();
    painter->setPen(QPen(edgePen.color(), edgePen.widthF()));
    painter->setBrush(m_filled ? edgePen.color() : Qt::NoBrush);
    painter->drawEllipse(center, radius, radius);
    painter->restore();

    return insetPoint(tip, angle, radius * 2);
}

qreal CircleTerminus::size() const { return kArrowSize * 0.8; }

// --- BarTerminus ---

QPointF BarTerminus::draw(QPainter *painter, const QPointF &tip,
                           const QPointF &from, const QPen &edgePen)
{
    const double angle = directionAngle(tip, from);
    const double perpAngle = angle + M_PI / 2.0;
    const double halfLen = kArrowSize * 0.5;

    const QPointF p1 = tip + QPointF(std::cos(perpAngle) * halfLen,
                                     -std::sin(perpAngle) * halfLen);
    const QPointF p2 = tip - QPointF(std::cos(perpAngle) * halfLen,
                                     -std::sin(perpAngle) * halfLen);

    painter->save();
    painter->setPen(QPen(edgePen.color(), edgePen.widthF() + 1, Qt::SolidLine,
                         Qt::FlatCap));
    painter->drawLine(p1, p2);
    painter->restore();

    return tip;
}

qreal BarTerminus::size() const { return kArrowSize * 0.5; }

// --- NoTerminus ---

QPointF NoTerminus::draw(QPainter * /*painter*/, const QPointF &tip,
                          const QPointF & /*from*/, const QPen & /*edgePen*/)
{
    return tip;
}

qreal NoTerminus::size() const { return 0.0; }

} // namespace Graffodil
