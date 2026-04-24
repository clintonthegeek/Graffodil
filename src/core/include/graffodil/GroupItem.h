#pragma once

#include <graffodil/GroupStyle.h>
#include <QGraphicsObject>
#include <QRectF>
#include <QString>

namespace Graffodil {

/// Decorative backdrop for a set of member nodes. Drawn as a rounded
/// rect (or sharp rect if cornerRadius == 0) padded around the union
/// of the members' bounding rects, with an optional title.
///
/// The item is purely decorative: shape() returns an empty path so
/// hit-testing never returns this item. GraphScene owns group
/// membership and calls setBounds() whenever the scene's "changed"
/// signal fires; GroupItem itself just holds the rect and paints it.
class GroupItem : public QGraphicsObject {
    Q_OBJECT
public:
    GroupItem(const QString &groupId,
              const GroupStyle &style,
              QGraphicsItem *parent = nullptr);

    QString groupId() const { return m_groupId; }

    const GroupStyle &style() const { return m_style; }
    void setStyle(const GroupStyle &style);

    /// Outer rect in scene coordinates (padding already applied).
    /// Pass a null rect to collapse the region.
    void setBounds(const QRectF &outer);
    QRectF bounds() const { return m_bounds; }

    QRectF boundingRect() const override { return m_bounds; }
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    /// Decorative: never participates in hit testing.
    QPainterPath shape() const override { return {}; }

private:
    QString    m_groupId;
    GroupStyle m_style;
    QRectF     m_bounds;
};

} // namespace Graffodil
