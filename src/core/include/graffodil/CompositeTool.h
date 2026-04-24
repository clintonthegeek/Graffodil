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

    /// Route left-button presses within `hitRadius` of any scene node's
    /// anchors to the given sub-tool. This enables anchor-initiated
    /// interactions (e.g., click-drag from an anchor to create an edge)
    /// without needing a separate tool mode. The route is prepended, so
    /// it takes priority over subsequent mouse routes.
    ///
    /// The matcher looks at every node's anchors() at press time, so it
    /// adapts to dynamic anchor sets (port nodes, swivel nodes, etc.).
    void addAnchorRoute(GraphTool *tool, qreal hitRadius = 12.0);

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

    // When an anchor route is installed, we also track hover to swap the
    // viewport cursor to PointingHand near an anchor (edge-creation affordance)
    // and leave it untouched otherwise so the node's own cursor shows through.
    // The same radius is used for the click-to-start-edge hit test and the
    // cursor-change feedback so they always agree spatially.
    qreal m_anchorHitRadius = 0.0;       // 0 disables anchor routing & hover
    bool m_anchorCursorActive = false;   // did we install the PointingHand cursor
    bool isNearAnchor(const QPointF &scenePos) const;
    void updateHoverCursor(const QPointF &scenePos);
};

} // namespace Graffodil
