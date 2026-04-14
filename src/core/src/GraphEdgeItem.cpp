#include <graffodil/GraphEdgeItem.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/EdgePathStrategy.h>
#include <graffodil/TerminusStyle.h>
#include <QPainter>
#include <QPainterPathStroker>
#include <QCursor>
#include <QUuid>

namespace Graffodil {

GraphEdgeItem::GraphEdgeItem(IGraphNode *source, Side sourceSide,
                             IGraphNode *target, Side targetSide,
                             EdgePathStrategy *strategy)
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_source(source)
    , m_target(target)
    , m_sourceSide(sourceSide)
    , m_targetSide(targetSide)
    , m_strategy(strategy)
{
    setFlag(ItemIsSelectable);
    // Hovering an edge should signal that it's clickable.
    setCursor(Qt::PointingHandCursor);
    setAcceptHoverEvents(true);
}

GraphEdgeItem::~GraphEdgeItem() = default;

QString GraphEdgeItem::edgeId() const { return m_id; }
IGraphNode *GraphEdgeItem::sourceNode() const { return m_source; }
IGraphNode *GraphEdgeItem::targetNode() const { return m_target; }
QGraphicsItem *GraphEdgeItem::graphicsItem() { return this; }

void GraphEdgeItem::setPathStrategy(EdgePathStrategy *strategy)
{
    m_strategy.reset(strategy);
}

void GraphEdgeItem::setTerminus(ArrowEnd end, std::unique_ptr<TerminusStyle> style)
{
    if (end == ArrowEnd::Source)
        m_sourceTerminus = std::move(style);
    else
        m_targetTerminus = std::move(style);
}

void GraphEdgeItem::setPen(const QPen &pen)
{
    QGraphicsPathItem::setPen(pen);
}

void GraphEdgeItem::setHighlighted(bool highlighted)
{
    m_highlighted = highlighted;
    update();
}

void GraphEdgeItem::setWaypoints(const QList<QPointF> &waypoints)
{
    m_waypoints = waypoints;
}

void GraphEdgeItem::reverse()
{
    std::swap(m_source, m_target);
    std::swap(m_sourceSide, m_targetSide);
    // Terminus styles stay attached to their ends (source-end vs target-end
    // as semantic ends, not endpoints), so swap them to preserve visual intent.
    std::swap(m_sourceTerminus, m_targetTerminus);
    if (!m_waypoints.isEmpty()) {
        QList<QPointF> reversed;
        reversed.reserve(m_waypoints.size());
        for (int i = m_waypoints.size() - 1; i >= 0; --i)
            reversed.append(m_waypoints[i]);
        m_waypoints = reversed;
    }
    adjust();
}

void GraphEdgeItem::adjust()
{
    if (!m_strategy || !m_source || !m_target)
        return;

    prepareGeometryChange();

    const QPointF srcAnchor = m_source->anchorPoint(m_sourceSide);
    const QPointF tgtAnchor = m_target->anchorPoint(m_targetSide);

    QPainterPath path = m_strategy->computePath(
        srcAnchor, m_sourceSide, tgtAnchor, m_targetSide, m_waypoints);

    setPath(path);
}

QRectF GraphEdgeItem::boundingRect() const
{
    // Expand the path's bounding rect to include terminus shapes,
    // so the full edge (including arrowheads) is invalidated on change.
    QRectF r = QGraphicsPathItem::boundingRect();
    qreal margin = m_hitWidth / 2.0;
    if (m_sourceTerminus)
        margin = qMax(margin, m_sourceTerminus->size());
    if (m_targetTerminus)
        margin = qMax(margin, m_targetTerminus->size());
    return r.adjusted(-margin, -margin, margin, margin);
}

QPainterPath GraphEdgeItem::shape() const
{
    // Stroke the path with a fixed hit-width independent of pen width.
    // This gives a predictable, narrow clickable area along the curve
    // regardless of path strategy or visual line thickness.
    if (path().isEmpty())
        return {};
    QPainterPathStroker stroker;
    stroker.setWidth(m_hitWidth);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    return stroker.createStroke(path());
}

void GraphEdgeItem::setHitWidth(qreal width)
{
    if (qFuzzyCompare(m_hitWidth, width))
        return;
    prepareGeometryChange();
    m_hitWidth = width;
}

void GraphEdgeItem::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (path().isEmpty())
        return;

    painter->setRenderHint(QPainter::Antialiasing);

    QPen edgePen = pen();
    if (m_highlighted) {
        edgePen.setWidthF(edgePen.widthF() + 1.0);
    }
    if (isSelected()) {
        // Distinct selection color — pen width is increased so selection
        // reads clearly regardless of the underlying edge color.
        edgePen.setColor(QColor(0, 120, 215));
        edgePen.setWidthF(qMax(edgePen.widthF(), 1.0) + 1.5);
    }

    painter->setPen(edgePen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path());

    // Draw terminus styles using the path tangent at each endpoint,
    // so arrowheads align with the actual curve direction (not the
    // straight line from source to target).
    const QPainterPath &p = path();
    const qreal len = qMax(p.length(), 1.0);
    const QPointF srcAnchor = p.pointAtPercent(0.0);
    const QPointF tgtAnchor = p.pointAtPercent(1.0);

    if (m_sourceTerminus) {
        QPointF fromDir = p.pointAtPercent(qMin(0.05, 10.0 / len));
        m_sourceTerminus->draw(painter, srcAnchor, fromDir, edgePen);
    }

    if (m_targetTerminus) {
        QPointF fromDir = p.pointAtPercent(qMax(0.95, 1.0 - 10.0 / len));
        m_targetTerminus->draw(painter, tgtAnchor, fromDir, edgePen);
    }
}

} // namespace Graffodil
