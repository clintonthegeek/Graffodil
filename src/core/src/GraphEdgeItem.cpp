#include <graffodil/GraphEdgeItem.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/EdgePathStrategy.h>
#include <graffodil/TerminusStyle.h>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPathStroker>
#include <QCursor>
#include <QUuid>

namespace Graffodil {

GraphEdgeItem::GraphEdgeItem(IGraphNode *source, const QString &sourceAnchorId,
                             IGraphNode *target, const QString &targetAnchorId,
                             std::unique_ptr<EdgePathStrategy> strategy)
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_source(source)
    , m_target(target)
    , m_sourceAnchorId(sourceAnchorId)
    , m_targetAnchorId(targetAnchorId)
    , m_strategy(std::move(strategy))
{
    setFlag(ItemIsSelectable);
    // Hovering an edge should signal that it's clickable.
    setCursor(Qt::PointingHandCursor);
    setAcceptHoverEvents(true);
}

GraphEdgeItem::GraphEdgeItem(IGraphNode *source, const QString &sourceAnchorId,
                             IGraphNode *target, const QString &targetAnchorId,
                             EdgePathStrategy *strategy)
    : GraphEdgeItem(source, sourceAnchorId, target, targetAnchorId,
                    std::unique_ptr<EdgePathStrategy>(strategy))
{
}

GraphEdgeItem::~GraphEdgeItem() = default;

QString GraphEdgeItem::edgeId() const { return m_id; }
IGraphNode *GraphEdgeItem::sourceNode() const { return m_source; }
IGraphNode *GraphEdgeItem::targetNode() const { return m_target; }
QGraphicsItem *GraphEdgeItem::graphicsItem() { return this; }

void GraphEdgeItem::setPathStrategy(std::unique_ptr<EdgePathStrategy> strategy)
{
    m_strategy = std::move(strategy);
}

void GraphEdgeItem::setPathStrategy(EdgePathStrategy *strategy)
{
    setPathStrategy(std::unique_ptr<EdgePathStrategy>(strategy));
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

void GraphEdgeItem::setGradient(const QColor &fromColor, const QColor &toColor)
{
    m_gradient = std::make_pair(fromColor, toColor);
    update();
}

void GraphEdgeItem::clearGradient()
{
    if (!m_gradient.has_value())
        return;
    m_gradient.reset();
    update();
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
    std::swap(m_sourceAnchorId, m_targetAnchorId);
    // Intentionally DO NOT swap termini. Termini are associated with the
    // semantic source/target slots, so keeping them in place means the
    // target terminus (e.g. an arrowhead) moves to the new target end —
    // which is the visible effect users expect from "reverse direction".
    // Swapping termini would leave the arrow visually stuck at its
    // previous node, making the reverse appear to do nothing.
    if (!m_waypoints.isEmpty()) {
        QList<QPointF> reversed;
        reversed.reserve(m_waypoints.size());
        for (int i = m_waypoints.size() - 1; i >= 0; --i)
            reversed.append(m_waypoints[i]);
        m_waypoints = reversed;
    }
    adjust();
}

Anchor GraphEdgeItem::resolveSourceAnchor() const
{
    if (m_sourceAnchorId.isEmpty()) {
        // Dynamic — toward the target's current position.
        const QPointF towards = m_target
            ? (m_targetAnchorId.isEmpty()
                ? m_target->nodeBoundingRect().center()
                : m_target->anchor(m_targetAnchorId).scenePos)
            : QPointF();
        return m_source->anchorToward(towards);
    }
    return m_source->anchor(m_sourceAnchorId);
}

Anchor GraphEdgeItem::resolveTargetAnchor() const
{
    if (m_targetAnchorId.isEmpty()) {
        // Dynamic — toward the source's current position. Use the source's
        // resolved anchor (which may itself have been dynamic) for best fit.
        const QPointF towards = m_source
            ? resolveSourceAnchor().scenePos
            : QPointF();
        return m_target->anchorToward(towards);
    }
    return m_target->anchor(m_targetAnchorId);
}

void GraphEdgeItem::adjust()
{
    if (!m_strategy || !m_source || !m_target)
        return;

    prepareGeometryChange();

    const Anchor src = resolveSourceAnchor();
    const Anchor tgt = resolveTargetAnchor();

    QPainterPath path = m_strategy->computePath(src, tgt, m_waypoints);
    setPath(path);

    m_shapeDirty = true;
    if (path.isEmpty()) {
        m_pathLength = 1.0;
        m_srcPoint = m_tgtPoint = m_srcTangentRef = m_tgtTangentRef = QPointF();
        return;
    }
    m_pathLength = qMax(path.length(), 1.0);
    m_srcPoint = path.pointAtPercent(0.0);
    m_tgtPoint = path.pointAtPercent(1.0);
    m_srcTangentRef = path.pointAtPercent(qMin(0.05, 10.0 / m_pathLength));
    m_tgtTangentRef = path.pointAtPercent(qMax(0.95, 1.0 - 10.0 / m_pathLength));
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
    if (path().isEmpty())
        return {};
    if (m_shapeDirty) {
        QPainterPathStroker stroker;
        stroker.setWidth(m_hitWidth);
        stroker.setCapStyle(Qt::RoundCap);
        stroker.setJoinStyle(Qt::RoundJoin);
        m_shapeCache = stroker.createStroke(path());
        m_shapeDirty = false;
    }
    return m_shapeCache;
}

void GraphEdgeItem::setHitWidth(qreal width)
{
    if (qFuzzyCompare(m_hitWidth, width))
        return;
    prepareGeometryChange();
    m_hitWidth = width;
    m_shapeDirty = true;
}

void GraphEdgeItem::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (path().isEmpty())
        return;

    QPen edgePen = pen();
    if (m_highlighted) {
        edgePen.setWidthF(edgePen.widthF() + 1.0);
    }
    if (isSelected()) {
        edgePen.setColor(QColor(0, 120, 215));
        edgePen.setWidthF(qMax(edgePen.widthF(), 1.0) + 1.5);
    }

    const QPainterPath &p = path();

    // Selection override wins over the gradient so selected edges remain
    // obviously highlighted; terminus styles always use the solid pen color
    // so the arrowhead reads cleanly against the background.
    QPen strokePen = edgePen;
    if (m_gradient.has_value() && !isSelected()) {
        QLinearGradient grad(m_srcPoint, m_tgtPoint);
        grad.setColorAt(0.0, m_gradient->first);
        grad.setColorAt(1.0, m_gradient->second);
        strokePen.setBrush(QBrush(grad));
    }

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(strokePen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(p);

    // Draw terminus styles using the cached path tangent at each endpoint.
    if (m_sourceTerminus)
        m_sourceTerminus->draw(painter, m_srcPoint, m_srcTangentRef, edgePen);

    if (m_targetTerminus)
        m_targetTerminus->draw(painter, m_tgtPoint, m_tgtTangentRef, edgePen);
}

} // namespace Graffodil
