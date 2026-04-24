#include <graffodil/GroupItem.h>

#include <QPainter>
#include <QPen>
#include <QBrush>

namespace Graffodil {

GroupItem::GroupItem(const QString &groupId,
                     const GroupStyle &style,
                     QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_groupId(groupId)
    , m_style(style)
{
    setZValue(m_style.zValue);
    setAcceptedMouseButtons(Qt::NoButton); // decorative; pass events through
}

void GroupItem::setStyle(const GroupStyle &style)
{
    m_style = style;
    setZValue(m_style.zValue);
    update();
}

void GroupItem::setBounds(const QRectF &outer)
{
    if (outer == m_bounds)
        return;
    prepareGeometryChange();
    m_bounds = outer;
}

void GroupItem::paint(QPainter *painter,
                      const QStyleOptionGraphicsItem * /*option*/,
                      QWidget * /*widget*/)
{
    if (m_bounds.isEmpty())
        return;

    painter->setRenderHint(QPainter::Antialiasing, true);

    if (m_style.borderColor.alpha() > 0 && m_style.borderWidth > 0.0) {
        QPen pen(m_style.borderColor, m_style.borderWidth, m_style.borderStyle);
        painter->setPen(pen);
    } else {
        painter->setPen(Qt::NoPen);
    }

    if (m_style.fillColor.alpha() > 0)
        painter->setBrush(QBrush(m_style.fillColor));
    else
        painter->setBrush(Qt::NoBrush);

    if (m_style.cornerRadius > 0.0) {
        painter->drawRoundedRect(m_bounds,
                                 m_style.cornerRadius,
                                 m_style.cornerRadius);
    } else {
        painter->drawRect(m_bounds);
    }

    if (!m_style.title.isEmpty()) {
        painter->setPen(m_style.titleColor);
        const qreal pad = qMax<qreal>(4.0, m_style.padding * 0.25);
        QRectF titleRect = m_bounds.adjusted(pad, pad, -pad, -pad);
        painter->drawText(titleRect,
                          Qt::AlignTop | Qt::AlignLeft,
                          m_style.title);
    }
}

} // namespace Graffodil
