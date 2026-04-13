#include <graffodil/GraphEdgeItem.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/EdgePathStrategy.h>
#include <graffodil/TerminusStyle.h>
#include <QPainter>
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
        edgePen.setColor(edgePen.color().lighter(150));
    }

    painter->setPen(edgePen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path());

    const QPointF srcAnchor = m_source->anchorPoint(m_sourceSide);
    const QPointF tgtAnchor = m_target->anchorPoint(m_targetSide);

    if (m_sourceTerminus) {
        QPointF fromDir = tgtAnchor;
        m_sourceTerminus->draw(painter, srcAnchor, fromDir, edgePen);
    }

    if (m_targetTerminus) {
        m_targetTerminus->draw(painter, tgtAnchor, srcAnchor, edgePen);
    }
}

} // namespace Graffodil
