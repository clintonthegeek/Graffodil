#include <graffodil/CompositeTool.h>
#include <graffodil/GraphScene.h>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QKeyEvent>
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
