#pragma once

#include <QPainter>
#include <QPointF>
#include <QPen>

namespace Graffodil {

class TerminusStyle {
public:
    virtual ~TerminusStyle() = default;
    virtual QPointF draw(QPainter *painter, const QPointF &tip,
                         const QPointF &from, const QPen &edgePen) = 0;
    virtual qreal size() const = 0;
};

class TriangleTerminus : public TerminusStyle {
public:
    QPointF draw(QPainter *painter, const QPointF &tip,
                 const QPointF &from, const QPen &edgePen) override;
    qreal size() const override;
};

class OpenArrowTerminus : public TerminusStyle {
public:
    QPointF draw(QPainter *painter, const QPointF &tip,
                 const QPointF &from, const QPen &edgePen) override;
    qreal size() const override;
};

class DiamondTerminus : public TerminusStyle {
public:
    explicit DiamondTerminus(bool filled = true) : m_filled(filled) {}
    QPointF draw(QPainter *painter, const QPointF &tip,
                 const QPointF &from, const QPen &edgePen) override;
    qreal size() const override;
private:
    bool m_filled;
};

class CircleTerminus : public TerminusStyle {
public:
    explicit CircleTerminus(bool filled = true) : m_filled(filled) {}
    QPointF draw(QPainter *painter, const QPointF &tip,
                 const QPointF &from, const QPen &edgePen) override;
    qreal size() const override;
private:
    bool m_filled;
};

class BarTerminus : public TerminusStyle {
public:
    QPointF draw(QPainter *painter, const QPointF &tip,
                 const QPointF &from, const QPen &edgePen) override;
    qreal size() const override;
};

class NoTerminus : public TerminusStyle {
public:
    QPointF draw(QPainter *painter, const QPointF &tip,
                 const QPointF &from, const QPen &edgePen) override;
    qreal size() const override;
};

} // namespace Graffodil
