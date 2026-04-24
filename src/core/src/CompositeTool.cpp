#include <graffodil/CompositeTool.h>
#include <graffodil/CreateEdgeTool.h>
#include <graffodil/GraphScene.h>
#include <graffodil/IGraphNode.h>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QLineF>
#include <QSet>

namespace Graffodil {

void CompositeTool::addMouseRoute(GraphTool *tool, MouseMatcher matcher)
{
    if (tool)
        m_mouseRoutes.append({tool, std::move(matcher)});
}

void CompositeTool::addWheelRoute(GraphTool *tool, WheelMatcher matcher)
{
    if (tool)
        m_wheelRoutes.append({tool, std::move(matcher)});
}

void CompositeTool::addKeyRoute(GraphTool *tool)
{
    if (tool && !m_keyRoutes.contains(tool))
        m_keyRoutes.append(tool);
}

void CompositeTool::addFallbackMouseRoute(GraphTool *tool)
{
    addMouseRoute(tool, matchAnyButton());
}

void CompositeTool::addAnchorRoute(GraphTool *tool, qreal hitRadius)
{
    if (!tool)
        return;
    // Remember the radius — the same value is used for click routing,
    // hover-cursor feedback, and (via CreateEdgeTool below) anchor-dot
    // highlighting, so all three transition at the same boundary. If
    // multiple routes are added (unusual), the largest wins.
    m_anchorHitRadius = qMax(m_anchorHitRadius, hitRadius);
    // Matcher uses the shared predicate so routing and cursor stay in lockstep.
    auto matcher = [this](QGraphicsSceneMouseEvent *e) {
        return e->button() == Qt::LeftButton && isNearAnchor(e->scenePos());
    };
    // Prepend — anchor routing must beat generic Left-button routes.
    m_mouseRoutes.prepend({tool, matcher});
    // Keep the routed CreateEdgeTool's hover highlight radius in lockstep
    // with the routing hit radius so the dots appear at the same boundary.
    if (auto *createEdge = qobject_cast<CreateEdgeTool *>(tool))
        createEdge->setAnchorHoverRadius(hitRadius);
}

bool CompositeTool::isNearAnchor(const QPointF &scenePos) const
{
    if (m_anchorHitRadius <= 0.0 || !m_scene)
        return false;
    for (auto *node : m_scene->nodes()) {
        // Cheap AABB reject before iterating anchors.
        QRectF bounds = node->graphicsItem()->sceneBoundingRect()
                            .adjusted(-m_anchorHitRadius, -m_anchorHitRadius,
                                      m_anchorHitRadius, m_anchorHitRadius);
        if (!bounds.contains(scenePos))
            continue;
        for (const Anchor &a : node->anchors()) {
            if (QLineF(a.scenePos, scenePos).length() <= m_anchorHitRadius)
                return true;
        }
    }
    return false;
}

void CompositeTool::updateHoverCursor(const QPointF &scenePos)
{
    if (m_anchorHitRadius <= 0.0 || !m_scene || m_scene->views().isEmpty())
        return;

    const bool nearAnchor = isNearAnchor(scenePos);
    auto *viewport = m_scene->views().first()->viewport();
    if (nearAnchor && !m_anchorCursorActive) {
        // Override the node's own OpenHand cursor with the pointing finger
        // to signal "click-and-drag here to create an edge".
        viewport->setCursor(Qt::PointingHandCursor);
        m_anchorCursorActive = true;
    } else if (!nearAnchor && m_anchorCursorActive) {
        // Fall back to whatever cursor the item under the mouse wants
        // (OpenHand on node bodies, default on empty canvas).
        viewport->unsetCursor();
        m_anchorCursorActive = false;
    }
}

CompositeTool::MouseMatcher CompositeTool::matchButton(
    Qt::MouseButton button, Qt::KeyboardModifiers mods)
{
    return [button, mods](QGraphicsSceneMouseEvent *e) {
        return e->button() == button
            && (mods == Qt::NoModifier
                ? (e->modifiers() & ~Qt::KeypadModifier) == Qt::NoModifier
                : (e->modifiers() & mods) == mods);
    };
}

CompositeTool::MouseMatcher CompositeTool::matchAnyButton()
{
    return [](QGraphicsSceneMouseEvent *) { return true; };
}

CompositeTool::WheelMatcher CompositeTool::matchWheelModifier(Qt::KeyboardModifiers mods)
{
    return [mods](QGraphicsSceneWheelEvent *e) {
        if (mods == Qt::NoModifier)
            return (e->modifiers() & ~Qt::KeypadModifier) == Qt::NoModifier;
        return (e->modifiers() & mods) == mods;
    };
}

CompositeTool::WheelMatcher CompositeTool::matchAnyWheel()
{
    return [](QGraphicsSceneWheelEvent *) { return true; };
}

void CompositeTool::activate(GraphScene *scene)
{
    GraphTool::activate(scene);
    // Collect the unique set of sub-tools across all routes and activate each.
    QSet<GraphTool *> seen;
    auto activateOnce = [&](GraphTool *t) {
        if (t && !seen.contains(t)) {
            seen.insert(t);
            t->activate(scene);
        }
    };
    for (const auto &r : m_mouseRoutes) activateOnce(r.tool);
    for (const auto &r : m_wheelRoutes) activateOnce(r.tool);
    for (auto *t : m_keyRoutes) activateOnce(t);
}

void CompositeTool::deactivate()
{
    QSet<GraphTool *> seen;
    auto deactivateOnce = [&](GraphTool *t) {
        if (t && !seen.contains(t)) {
            seen.insert(t);
            t->deactivate();
        }
    };
    for (const auto &r : m_mouseRoutes) deactivateOnce(r.tool);
    for (const auto &r : m_wheelRoutes) deactivateOnce(r.tool);
    for (auto *t : m_keyRoutes) deactivateOnce(t);
    m_dragSubTool = nullptr;
    // Don't leave the PointingHand cursor installed after the tool goes away.
    if (m_anchorCursorActive && m_scene && !m_scene->views().isEmpty()) {
        m_scene->views().first()->viewport()->unsetCursor();
        m_anchorCursorActive = false;
    }
    GraphTool::deactivate();
}

void CompositeTool::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    for (const auto &r : m_mouseRoutes) {
        if (r.matcher && r.matcher(event)) {
            m_dragSubTool = r.tool;
            r.tool->mousePressEvent(event);
            return;
        }
    }
}

void CompositeTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // While a gesture is active, route move to the sub-tool that handled press.
    if (m_dragSubTool) {
        m_dragSubTool->mouseMoveEvent(event);
        return;
    }
    // Otherwise (hover-style moves with no buttons pressed), broadcast to all
    // sub-tools. This keeps CreateEdgeTool's live preview line working when
    // it's embedded as a sub-tool (though we don't do that by default).
    for (const auto &r : m_mouseRoutes)
        r.tool->mouseMoveEvent(event);
    updateHoverCursor(event->scenePos());
}

void CompositeTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_dragSubTool) {
        m_dragSubTool->mouseReleaseEvent(event);
        m_dragSubTool = nullptr;
    }
}

void CompositeTool::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    for (const auto &r : m_wheelRoutes) {
        if (r.matcher && r.matcher(event)) {
            r.tool->wheelEvent(event);
            if (event->isAccepted())
                return;
        }
    }
}

void CompositeTool::keyPressEvent(QKeyEvent *event)
{
    for (auto *t : m_keyRoutes) {
        t->keyPressEvent(event);
        if (event->isAccepted())
            return;
    }
}

void CompositeTool::keyReleaseEvent(QKeyEvent *event)
{
    for (auto *t : m_keyRoutes) {
        t->keyReleaseEvent(event);
        if (event->isAccepted())
            return;
    }
}

QList<GraphTool *> CompositeTool::allSubTools() const
{
    QSet<GraphTool *> seen;
    QList<GraphTool *> result;
    auto addOnce = [&](GraphTool *t) {
        if (t && !seen.contains(t)) {
            seen.insert(t);
            result.append(t);
        }
    };
    for (const auto &r : m_mouseRoutes) addOnce(r.tool);
    for (const auto &r : m_wheelRoutes) addOnce(r.tool);
    for (auto *t : m_keyRoutes) addOnce(t);
    return result;
}

} // namespace Graffodil
