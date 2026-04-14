#pragma once

#include <graffodil/GraphTool.h>
#include <QList>
#include <functional>

namespace Graffodil {

/// A tool that routes events to sub-tools based on button/modifier predicates.
///
/// Each sub-tool is registered with an EventMatcher that decides whether the
/// sub-tool should handle a given event. The first matching sub-tool wins.
///
/// Once a sub-tool handles a mousePressEvent, subsequent move/release events
/// are routed to that same sub-tool until the gesture ends — this preserves
/// drag state across the event sequence.
///
/// Wheel and key events are dispatched by matcher on every event (no state).
///
/// Sub-tools remain owned by the consumer; CompositeTool does not delete them.
class CompositeTool : public GraphTool {
    Q_OBJECT
public:
    using GraphTool::GraphTool;

    using MouseMatcher = std::function<bool(QGraphicsSceneMouseEvent *)>;
    using WheelMatcher = std::function<bool(QGraphicsSceneWheelEvent *)>;

    /// Add a sub-tool that handles mouse press events matching the predicate.
    /// Move/release events are routed to whichever sub-tool handled the press.
    void addMouseRoute(GraphTool *tool, MouseMatcher matcher);

    /// Add a sub-tool that handles wheel events matching the predicate.
    void addWheelRoute(GraphTool *tool, WheelMatcher matcher);

    /// Add a sub-tool that receives all key events.
    /// Multiple sub-tools can be added; each is tried until one accepts.
    void addKeyRoute(GraphTool *tool);

    /// Convenience: add a sub-tool that handles all unmatched mouse events.
    /// Matcher is a catch-all. Must be added LAST (after specific matchers).
    void addFallbackMouseRoute(GraphTool *tool);

    // --- Convenience matcher factories ---

    static MouseMatcher matchButton(Qt::MouseButton button,
                                    Qt::KeyboardModifiers mods = Qt::NoModifier);
    static MouseMatcher matchAnyButton();
    static WheelMatcher matchWheelModifier(Qt::KeyboardModifiers mods);
    static WheelMatcher matchAnyWheel();

    // --- GraphTool ---

    void activate(GraphScene *scene) override;
    void deactivate() override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

protected:
    /// Access the sub-tools for subclasses that want to expose them.
    QList<GraphTool *> allSubTools() const;

private:
    struct MouseRoute {
        GraphTool *tool = nullptr;
        MouseMatcher matcher;
    };
    struct WheelRoute {
        GraphTool *tool = nullptr;
        WheelMatcher matcher;
    };

    QList<MouseRoute> m_mouseRoutes;
    QList<WheelRoute> m_wheelRoutes;
    QList<GraphTool *> m_keyRoutes;
    GraphTool *m_dragSubTool = nullptr;  // the sub-tool currently handling a drag
};

} // namespace Graffodil
