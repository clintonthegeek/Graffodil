#pragma once

#include <QColor>
#include <QString>
#include <Qt>

namespace Graffodil {

/// Visual style for a group region drawn behind a set of member nodes.
/// All fields are optional; defaults produce a subtle dashed outline
/// with no fill, placed beneath nodes and edges.
struct GroupStyle {
    QColor borderColor    = Qt::transparent;
    qreal  borderWidth    = 1.0;
    Qt::PenStyle borderStyle = Qt::DashLine;
    QColor fillColor      = Qt::transparent;

    /// Rounded-rect corner radius. Set 0 for a sharp-cornered rect.
    qreal cornerRadius    = 8.0;

    /// Halo (scene units) between the union of member bounding rects
    /// and the drawn region's outer edge.
    qreal padding         = 16.0;

    /// Optional label rendered at the top-left of the region. Empty
    /// string suppresses drawing.
    QString title;
    QColor  titleColor    = Qt::white;

    /// QGraphicsItem::zValue for the region. Default puts it below
    /// nodes (z=0) and edges (z=-1).
    qreal   zValue        = -2.0;
};

} // namespace Graffodil
