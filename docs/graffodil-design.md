# Graffodil — Shared Qt Graph Scene Framework

**Date:** 2026-04-13
**Status:** Design approved, pending implementation plan
**Repository:** `~/dev/Graffodil/` (standalone, consumed by PlanStan and Corbomite)

## Problem

Five graph drawing contexts exist across two projects (PlanStan and Corbomite), each independently implementing QGraphicsView/Scene-based graph rendering with overlapping patterns: arrowhead drawing, anchor systems, tool state machines, edge routing. This duplication increases maintenance burden and prevents cross-pollination of improvements.

| Context | Project | LOC | Scale | Layout | Edge Style |
|---------|---------|-----|-------|--------|------------|
| PERT/CPM graph | PlanStan `libs/graph/` | ~5,768 | ~100s | Sugiyama layered | Orthogonal/Direct |
| Gantt arrows | PlanStan `libs/gantt/` | ~300 | ~100s | N/A (1D constrained) | Dog-leg polyline |
| Sync topology | PlanStan (not yet built) | — | 3–7 | Circular/Force | Directed arrows |
| Force-directed graph | Corbomite `libs/forcegraph/` | ~3,283 | 10K+ | ForceAtlas2 + multilevel | Straight lines |
| Canvas | Corbomite `libs/canvas/` | ~3,851 | ~100s | Manual placement | Cubic bezier |

## Goals

- Eliminate duplication of graph primitives (arrowheads, anchors, tools, edge rendering)
- Provide a scene framework flexible enough for all five contexts without constraining any of them
- Keep the core lightweight — optional modules for layout algorithms and high-node-count rendering
- Enable building the sync topology widget on Graffodil from day one (proving the framework)

## Non-Goals

- Replacing the Gantt arrow system (too 1D-specific; borrows arrowhead math only)
- Replacing PlanStan's `DependencyGraph` CPM engine (stays in `libs/scheduling/`)
- Dictating what nodes look like (consumers own node rendering)
- Forcing all consumers to carry layout or batch rendering code they don't need

## Architecture

### Core + Extensions Model

Mirrors Qt's own component packaging. One core library containing the scene framework, with optional satellite targets for layout algorithms and batch rendering.

```
Graffodil::Core       — interfaces, scene, tools, edge items, path strategies,
                         terminus styles, minimap, anchor highlight, connector
Graffodil::Spatial    — QuadTree, SpatialGrid (depends: Qt6::Core only)
Graffodil::Sugiyama   — Sugiyama layered layout engine (depends: Core)
Graffodil::Force      — ForceAtlas2 + multilevel layout (depends: Core, Spatial)
Graffodil::Circular   — Circular/radial layout (depends: Core)
Graffodil::Batch      — BatchRenderer (depends: Core)
```

Consumer CMake:
```cmake
find_package(Graffodil COMPONENTS Core Sugiyama REQUIRED)
target_link_libraries(my-target PRIVATE Graffodil::Core Graffodil::Sugiyama)
```

### Design Strategy

**Pragmatic hybrid:** clean-room the interfaces and contracts, but transplant proven implementations from existing code. The Sugiyama layout, ForceAtlas2 engine, bezier edge renderer, and batch rendering pipeline are battle-tested — no rewrites.

---

## Core Interfaces

### IGraphNode

Anything that can participate as a node in a `GraphScene`. Consumers implement this on their own QGraphicsItem subclasses.

```cpp
namespace Graffodil {

enum class Side { Top, Right, Bottom, Left };

class IGraphNode {
public:
    virtual ~IGraphNode() = default;

    // Unique identifier for this node within the scene.
    virtual QString nodeId() const = 0;

    // Connection point for the given side. Used by edges and tools
    // to determine where to attach/draw from.
    virtual QPointF anchorPoint(Side side) const = 0;

    // Bounding rect in scene coordinates. Used by the scene for
    // spatial queries and by layout engines for spacing.
    virtual QRectF nodeBoundingRect() const = 0;

    // Bridge to the Qt graphics item system. The scene calls this
    // to add/remove the item from the QGraphicsScene.
    // Must return a stable pointer for the lifetime of the node.
    virtual QGraphicsItem *graphicsItem() = 0;
};

}
```

**ID type rationale:** `QString` chosen because all existing consumers already use string IDs (UUIDs in PlanStan, file paths in Corbomite ForceGraph, hex strings in Corbomite Canvas). Hash lookup performance is sufficient for the core scene's scale. The Batch module maintains its own packed integer arrays internally where performance demands it.

### IGraphEdge

Anything that can participate as an edge. Consumers can use the provided `GraphEdgeItem` or implement their own.

```cpp
namespace Graffodil {

class IGraphEdge {
public:
    virtual ~IGraphEdge() = default;

    virtual QString edgeId() const = 0;
    virtual IGraphNode *sourceNode() const = 0;
    virtual IGraphNode *targetNode() const = 0;

    // Fast-path endpoint ids — default-implemented in terms of
    // sourceNode()/targetNode(), returns empty string if null.
    // Useful in tight loops (hover trace, edge dispatch) where a
    // full dereference is unnecessary.
    QString sourceNodeId() const;
    QString targetNodeId() const;

    // Bridge to QGraphicsItem, same pattern as IGraphNode.
    virtual QGraphicsItem *graphicsItem() = 0;

    // Notification that either endpoint node may have moved or changed
    // geometry. The scene calls this from adjustEdgesForNode(),
    // adjustAllEdges(), and applyLayout(). Implementations should recompute
    // their visual path. Default is a no-op so data-only edges stay valid.
    virtual void endpointsChanged() {}

    // Routing hints delivered by GraphScene::applyLayout() when a layout
    // engine produced LayoutResult::edgeWaypoints for this edge's
    // (sourceId, targetId) pair. Scene coordinates, ordered source→target.
    // Default ignores them; GraphEdgeItem feeds them to its path strategy.
    // Known limitation: parallel edges between the same pair share the first
    // matching waypoint list.
    virtual void setWaypoints(const QList<QPointF> &waypoints)
    {
        Q_UNUSED(waypoints);
    }
};

}
```

### LayoutResult

The communication contract between layout engines and the scene. Layout engines produce this; the scene consumes it via `applyLayout()`.

```cpp
namespace Graffodil {

struct LayoutResult {
    QHash<QString, QPointF> nodePositions;
    // Optional: waypoints for edges that span multiple layers (Sugiyama)
    // or need routing hints. Key is (sourceId, targetId).
    QHash<QPair<QString, QString>, QList<QPointF>> edgeWaypoints;
};

}
```

---

## GraphScene

The central framework piece. Manages node/edge registry, dispatches events to the active tool, provides typed selection queries.

```cpp
namespace Graffodil {

class GraphScene : public QGraphicsScene {
    Q_OBJECT
public:
    // --- Node/edge registry ---

    void addNode(IGraphNode *node);
    void removeNode(const QString &id);
    IGraphNode *nodeForId(const QString &id) const;
    QList<IGraphNode *> nodes() const;

    void addEdge(IGraphEdge *edge);
    void removeEdge(const QString &id);
    IGraphEdge *edgeForId(const QString &id) const;
    IGraphEdge *edgeBetween(const QString &sourceId, const QString &targetId) const;
    IGraphEdge *edgeBetweenUndirected(const QString &nodeId1, const QString &nodeId2) const;
    QList<IGraphEdge *> connectedEdges(const QString &nodeId) const;
    QList<IGraphEdge *> edges() const;

    // --- Adjacency index ---
    // Efficient per-node edge lookups via internal QMultiHash index.
    // connectedEdges(), edgeBetween(), edgeBetweenUndirected(), and adjustEdgesForNode()
    // are all O(degree) instead of O(total_edges). Index is maintained automatically
    // on addEdge()/removeEdge(). Note: an edge's source and target nodeIds must remain
    // stable while the edge is registered; to change endpoints use removeEdge() followed
    // by re-adding the edge with the new endpoint (no hot-swapping of nodeId references).

    // --- Tool dispatch ---

    // Consumer wires tool activation to QAction/KAction.
    // The scene doesn't know about action frameworks.
    void setActiveTool(GraphTool *tool);
    GraphTool *activeTool() const;

    // --- Selection (typed over interfaces) ---

    QList<IGraphNode *> selectedNodes() const;
    QList<IGraphEdge *> selectedEdges() const;

    // Id of the first registered node currently selected (or empty).
    QString selectedNodeId() const;

    // Clear selection and select the node with this id.
    void selectNode(const QString &id);

    // --- Group regions ---

    // A group is a decorative backdrop drawn behind a set of member
    // nodes — a rounded, padded rect (with optional fill/border/title)
    // that tracks the members as they move. Groups never intercept
    // mouse events on members; they are purely visual. Members may
    // overlap multiple groups (layer via GroupStyle::zValue). Removing
    // a node cascades into any group containing it.

    QString addGroup(const QSet<QString> &memberNodeIds,
                     const GroupStyle &style);
    void setGroupStyle  (const QString &groupId, const GroupStyle &style);
    void setGroupMembers(const QString &groupId, const QSet<QString> &memberNodeIds);
    void removeGroup    (const QString &groupId);

    QStringList  groupIds() const;
    QSet<QString> groupMembers(const QString &groupId) const;
    GroupItem    *groupItem(const QString &groupId) const;

    // Force an immediate synchronous refresh of all group bounds.
    // Useful for code that moves nodes programmatically outside the normal
    // event loop and needs groups to update immediately. Normally groups
    // auto-refresh once per event-loop turn; this flushes any pending update.
    void refreshGroupsNow();

    // --- Layout ---

    // Applies computed positions to registered nodes. Delivers routing
    // hints to edges before calling endpointsChanged() so recomputed paths
    // see the waypoints. Groups refresh their bounds synchronously after
    // the layout is applied.
    void applyLayout(const LayoutResult &result);

signals:
    void nodeAdded(IGraphNode *node);
    void nodeRemoved(const QString &id);
    void edgeAdded(IGraphEdge *edge);
    void edgeRemoved(const QString &id);
    void selectionChanged();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

    // Subclass hook for the empty-canvas double-click gesture.
    // Default is a no-op; called before backgroundDoubleClicked() emits.
    virtual void mouseDoubleClickEventBackground(const QPointF &scenePos);
}

// Background double-click fired by the default mouseDoubleClickEvent() when
// no graphics item is under the cursor. See "Background double-click gesture"
// below.
```

**Ownership model:** The scene manages the QGraphicsItem lifetime (standard Qt parent-child ownership via `QGraphicsScene::addItem()`). The `IGraphNode`/`IGraphEdge` wrapper objects are owned by the consumer — the scene holds non-owning pointers and removes them from its registry on `removeNode()`/`removeEdge()`.

**Layout agnosticism:** The scene does not require a layout engine. Consumers can position nodes manually (Canvas), via a one-shot layout (Sugiyama, Circular), via an animated simulation (Force), or by deserializing saved positions. The scene neither knows nor cares which approach is used.

### Background double-click gesture

The default `mouseDoubleClickEvent()` implementation centralizes the
empty-canvas gesture through two APIs:

| API | Use when… |
|-----|-----------|
| `backgroundDoubleClicked(const QPointF&)` signal | The consumer is not a `GraphScene` subclass, or multiple observers need to react. Connect a slot. |
| `mouseDoubleClickEventBackground(const QPointF&)` protected virtual | The consumer is a `GraphScene` subclass and wants a synchronous hook (e.g., call `placeNodeAt()` to create a new node at the double-click point). Override it. |

Both fire in the same call, in this order: the virtual is invoked first, then
the signal is emitted. Subclasses that override the virtual run before any
slot connected to the signal — so observers see the post-subclass state.
The virtual default is a no-op, so overriding it is purely additive: existing
consumers connected only to the signal see no behavior change.

### Edge-midpoint action affordances

`IGraphEdge::edgeActions()` returns a list of lightweight `EdgeAction{id, pos}`
descriptors.  The scene performs a small-radius hit-test around each declared
parametric position (computed via the edge's `QGraphicsPathItem` geometry) and,
on a match, first calls the protected virtual `mousePressEventEdgeAction` then
emits `edgeActionTriggered`.  This gives consumers a reusable surface for
mid-edge glyphs (insert node, delete edge, …) without each application
re-implementing the hit-test logic.  Concrete visuals and semantics remain
100 % consumer-owned.

**Do not** reproduce the empty-area test (`items(pos).isEmpty()`) in your own
`mouseDoubleClickEvent` override. The scene owns the definition of "is this
the empty canvas?" — duplicating it in each consumer leads to subtle drift
if the criterion ever changes (e.g., to exclude handles or zoom anchors).
If you need full mouse-event control, override `mouseDoubleClickEvent` and
forward to `QGraphicsScene::mouseDoubleClickEvent` after your custom logic.

### Group regions

Groups are styled backdrops — a rounded padded rect with optional fill, border, and title — drawn behind a set of member nodes. They support use cases like compound milestones, cluster highlighting, and sub-graph decoration.

```cpp
namespace Graffodil {

struct GroupStyle {
    QColor borderColor    = Qt::transparent;
    qreal  borderWidth    = 1.0;
    Qt::PenStyle borderStyle = Qt::DashLine;
    QColor fillColor      = Qt::transparent;
    qreal  cornerRadius   = 8.0;
    qreal  padding        = 16.0;  // halo around member bounds
    QString title;                 // empty = no label
    QColor  titleColor    = Qt::white;
    qreal   zValue        = -2.0;  // default: below nodes & edges
};

class GroupItem : public QGraphicsObject {
    // Backdrop drawable. shape() returns an empty path so the item
    // never participates in hit-testing — members remain fully
    // interactive. Consumers normally don't instantiate this directly;
    // GraphScene::addGroup() owns the lifetime.
};

} // namespace Graffodil
```

**Tracking:** GraphScene connects to its own `QGraphicsScene::changed` signal once the first group is added. Group refreshes are coalesced to at most once per event-loop turn — on each emission, a pending refresh is queued and deferred to the next idle moment rather than running immediately. This prevents excessive bounds recalculation when many nodes move in a single interaction. `applyLayout()` also performs a synchronous refresh so callers that query `groupItem(id)->bounds()` immediately after the layout see fresh data. For code that moves nodes programmatically outside the event-loop (e.g., scripted animations), call `refreshGroupsNow()` to force an immediate synchronous update of all group bounds. A recursion guard prevents re-entry while a refresh is in flight.

**Ownership:** GroupItems are owned by the scene — `removeGroup()` and `clearGraph()` delete them. Member ids in the registry are authoritative; resolving them to `IGraphNode*` happens lazily on every refresh, so nodes removed via `removeNode()` drop out of group membership automatically.

---

## Edge Rendering

### EdgePathStrategy

Pluggable path computation. Determines the shape of the line between two anchor points.

```cpp
namespace Graffodil {

class EdgePathStrategy {
public:
    virtual ~EdgePathStrategy() = default;

    // Compute the QPainterPath for an edge given its endpoints and
    // optional waypoints (from layout engines or manual routing).
    virtual QPainterPath computePath(
        const QPointF &sourceAnchor, Side sourceSide,
        const QPointF &targetAnchor, Side targetSide,
        const QList<QPointF> &waypoints = {}) = 0;
};

}
```

**Provided strategies** (transplanted from existing code):

| Strategy | Source | Behavior |
|----------|--------|----------|
| `DirectPathStrategy` | New (trivial) | Straight line from source to target |
| `BezierPathStrategy` | Corbomite `EdgeItem.cpp` | Cubic bezier with curvature = min(distance * 0.4, 80.0), control points extend outward from connection side |
| `OrthogonalPathStrategy` | PlanStan `DependencyEdgeItem` | Right-angle segments with waypoint support, channel-based routing to minimize overlaps |
| `PolylinePathStrategy` | PlanStan `GanttArrowItem` (adapted) | Multi-segment straight lines through waypoints |

### TerminusStyle

Pluggable endpoint rendering. Replaces a fixed arrowhead enum with extensible per-endpoint shapes.

```cpp
namespace Graffodil {

class TerminusStyle {
public:
    virtual ~TerminusStyle() = default;

    // Draw the terminus at 'tip', arriving from direction 'from'.
    // Returns the inset point where the edge line should end,
    // so the line meets the base of the shape rather than the tip.
    virtual QPointF draw(QPainter *painter, const QPointF &tip,
                         const QPointF &from, const QPen &edgePen) = 0;

    // Size contribution for bounding rect / hit testing.
    virtual qreal size() const = 0;
};

}
```

**Provided terminus styles:**

| Style | Visual |
|-------|--------|
| `TriangleTerminus` | Filled triangle (default arrowhead) |
| `OpenArrowTerminus` | Unfilled chevron |
| `DiamondTerminus` | Filled or unfilled diamond (configurable) |
| `CircleTerminus` | Filled or unfilled dot (configurable) |
| `BarTerminus` | Perpendicular bar |
| `NoTerminus` | Nothing (returns tip unchanged) |

Consumers can subclass `TerminusStyle` for domain-specific shapes (e.g., `CrowsFootTerminus` for ER diagrams) without modifying library code.

### GraphEdgeItem

The provided edge implementation. Consumers who don't need custom edge rendering use this directly. It implements `IGraphEdge` and composes a path strategy with terminus styles.

```cpp
namespace Graffodil {

enum class ArrowEnd { Source, Target };

class GraphEdgeItem : public QGraphicsPathItem, public IGraphEdge {
public:
    GraphEdgeItem(IGraphNode *source, Side sourceSide,
                  IGraphNode *target, Side targetSide,
                  std::unique_ptr<EdgePathStrategy> strategy);
    // Raw-pointer constructor is deprecated; ownership transfers either way.
    [[deprecated]] GraphEdgeItem(IGraphNode *source, Side sourceSide,
                                 IGraphNode *target, Side targetSide,
                                 EdgePathStrategy *strategy);

    // --- IGraphEdge ---
    QString edgeId() const override;
    IGraphNode *sourceNode() const override;
    IGraphNode *targetNode() const override;
    QGraphicsItem *graphicsItem() override;
    void endpointsChanged() override { adjust(); }
    void setWaypoints(const QList<QPointF> &waypoints) override;

    // --- Configuration ---
    // Takes ownership via unique_ptr (preferred) or raw pointer (deprecated).
    void setPathStrategy(std::unique_ptr<EdgePathStrategy> strategy);
    [[deprecated]] void setPathStrategy(EdgePathStrategy *strategy);
    void setTerminus(ArrowEnd end, std::unique_ptr<TerminusStyle> style);
    void setPen(const QPen &pen);      // line style, dash pattern, width, color
    void setHighlighted(bool);

    // --- Geometry update ---
    // Call when source/target nodes move. Recomputes path from
    // current anchor positions through the active strategy.
    void adjust();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
};

}
```

**Shared arrowhead math:** The `atan2` + polygon arrowhead code duplicated across all four existing views is written once in `GraphEdgeItem::paint()` (delegating to the active `TerminusStyle`). This is the single biggest duplication eliminated by the library.

---

## Tool Framework

Tools handle user interaction. They receive scene events, emit signals for consumer response, and never mutate the data model directly.

### GraphTool Base

```cpp
namespace Graffodil {

class GraphTool : public QObject {
    Q_OBJECT
public:
    // Called by GraphScene when this tool becomes active/inactive.
    // Use to set cursors, add preview items, etc.
    virtual void activate(GraphScene *scene);
    virtual void deactivate();

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *) = 0;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *) = 0;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *) = 0;
    virtual void keyPressEvent(QKeyEvent *) {}
    virtual void keyReleaseEvent(QKeyEvent *) {}

    GraphScene *scene() const;

signals:
    // Status hints for toolbar/statusbar display.
    // Consumer connects this to their status widget.
    void statusMessage(const QString &msg);
};

}
```

### Provided Tools

**SelectMoveTool** (~150 LOC, transplanted from PlanStan `SelectMoveTool` + Corbomite `CanvasTool`):
- Click: select node or edge
- Drag on node: move. Optional alignment via `IAlignmentStrategy` — consumers install a strategy via `setAlignmentStrategy()` to get snap behaviour and paint guide overlays; no default is shipped (consumers own the alignment policy).
- Rubber-band drag: multi-select
- Shift+click: toggle selection
- Delete key: emits `deleteRequested` (consumer decides what to delete and how to undo)

```cpp
class SelectMoveTool : public GraphTool {
    Q_OBJECT
signals:
    // Consumer handles deletion and undo. Tool doesn't mutate the scene.
    void deleteRequested(const QList<IGraphNode *> &nodes,
                         const QList<IGraphEdge *> &edges);
    // Consumer pushes undo commands for position changes.
    void nodeMoved(IGraphNode *node, const QPointF &oldPos, const QPointF &newPos);
};
```

**CreateEdgeTool** (~150 LOC, transplanted from PlanStan `CreateEdgeTool` + Corbomite canvas edge creation):
- Click near node edge: snap to nearest anchor, highlight
- Drag: dashed preview line following cursor
- Release on target node: emit `edgeRequested` with source/target and sides
- Both gestures are supported: click-click (click source, click target) and
  press-drag-release. A sub-threshold drag falls through into click-click,
  unless `setDragOnly(true)`.
- End on empty canvas: emit `edgeDroppedOnEmpty` (see the catalog below)
- Escape: cancel
- Prevents self-loops

```cpp
class CreateEdgeTool : public GraphTool {
    Q_OBJECT
signals:
    // Consumer creates the appropriate edge type, adds it to the scene,
    // updates their data model, and pushes an undo command.
    void edgeRequested(IGraphNode *source, Side sourceSide,
                       IGraphNode *target, Side targetSide);
};
```

**PanZoomTool** (~100 LOC, transplanted from both codebases):
- Left-drag: pan canvas
- Scroll / middle-drag: zoom to cursor position

```cpp
class PanZoomTool : public GraphTool {};
```

### Mutation signal catalog

Graffodil tools never mutate the data model directly — they emit signals and the consumer decides. This catalog lists every mutation-relevant signal each provided tool emits, so consumers wiring an undo stack have a complete picture.

**`SelectMoveTool`**

| Signal | When | Typical undo action |
|---|---|---|
| `dragBegan(nodes)` | Cursor crosses the 3-unit threshold after press-on-node; fires once per drag | Open an undo macro / compound command |
| `nodeMoved(node, oldPos, newPos)` | On mouse release, once per moved node in the drag set, only if the position actually changed | Push a move command into the open macro |
| `dragEnded(nodes)` | On mouse release, after all `nodeMoved` signals for the drag | Close the undo macro |
| `deleteRequested(nodes, edges)` | Delete / Backspace pressed while items are selected | Delete the items; push a delete command |
| `reverseRequested(edges)` | R pressed while edges are selected | Call `GraphEdgeItem::reverse()`; push a reverse command |

`dragEnded` fires regardless of whether any `nodeMoved` did — consumers should treat an empty `dragBegan`…`dragEnded` pair (e.g. user dragged past threshold then returned to the exact start position) as a no-op macro. `dragEnded` also fires if the tool is deactivated mid-drag (tool switch, scene teardown), again with no preceding `nodeMoved` — undo macros stay balanced.

Click-without-drag does not emit `dragBegan` / `dragEnded`. Multi-select drag emits one `dragBegan`, one `nodeMoved` per moved node, and one `dragEnded` — so a consumer gets one undo macro containing N move commands per user drag.

**`CreateEdgeTool`**

| Signal | When | Typical undo action |
|---|---|---|
| `edgeRequested(source, sourceAnchorId, target, targetAnchorId)` | User completes a click-click or drag gesture between two distinct nodes | Construct the edge, add it to scene + model, push a create command |
| `edgeDroppedOnEmpty(source, sourceAnchorId, scenePos)` | An in-progress gesture ends over empty canvas: drag released with no node under the cursor, or (click-click) the second click lands on the background | Offer "create a node here and connect it" — e.g. a context menu at `scenePos`; then create node + edge yourself and push one macro |

`edgeDroppedOnEmpty` fires *after* the tool has reset, so a handler may run a
modal menu or mutate the scene freely. Leaving it unconnected means
drop-on-empty just cancels the gesture. A drag that starts and ends on the
same node is a rejected self-loop, not a drop — it cancels silently.

**`GraphScene`**

| Signal | When |
|---|---|
| `nodeAdded(node)` / `nodeRemoved(id)` | `addNode` / `removeNode` called (structural — not user-intent) |
| `edgeAdded(edge)` / `edgeRemoved(id)` | `addEdge` / `removeEdge` called (structural — not user-intent) |
| `backgroundDoubleClicked(scenePos)` | User double-clicks on the empty canvas (no graphics item under cursor). Connect to react — or override `mouseDoubleClickEventBackground()` from a `GraphScene` subclass instead. |

Scene signals fire from the registry calls themselves, so they do not distinguish consumer-driven additions from tool-driven ones. For undo capture, connect to tool signals, not scene signals. (`backgroundDoubleClicked` is a user-intent signal, not a structural one — pair any "create on canvas" handler with an undo command of your own.)

**Not currently emitted (by design)**

- *Edge redirect* (dragging an existing edge's endpoint to a different node or anchor). No provided tool performs this yet; consumers implementing it should emit their own signal or use `GraphEdgeItem::setSourceAnchorId` / `setTargetAnchorId` through their own tool subclass.
- *Tool-activation-as-undo-unit*. Tool switching is UI state, not data mutation — no signal.

**Integration pattern:** Consumers wire tool activation to `QAction` or `KActionCollection` actions. The tools themselves are pure Qt — no KDE dependency in the library.

```cpp
// Example consumer integration (PlanStan):
// auto selectAction = actionCollection()->addAction("graph_select");
// connect(selectAction, &QAction::triggered, this, [this]() {
//     m_scene->setActiveTool(m_selectTool);
// });
```

---

## Layout Engines

### Interface (in Core)

```cpp
namespace Graffodil {

class LayoutEngine {
public:
    virtual ~LayoutEngine() = default;

    virtual void setNodes(const QList<IGraphNode *> &nodes) = 0;
    virtual void setEdges(const QList<IGraphEdge *> &edges) = 0;

    // Compute positions. Does not modify the scene.
    // Consumer calls scene->applyLayout(result).
    virtual LayoutResult compute() = 0;

    // For iterative layouts: run one simulation step.
    // Returns false when converged.
    virtual bool tick() { return false; }
    virtual bool isIterative() const { return false; }

    // Current positions after the last tick(). Only meaningful for
    // iterative layouts. One-shot layouts return empty until compute().
    virtual LayoutResult currentPositions() const { return {}; }
};

}
```

**Usage patterns:**

```cpp
// One-shot (Sugiyama, Circular):
scene->applyLayout(engine->compute());

// Animated (Force-directed):
// Consumer drives a QTimer:
//   if (!engine->tick()) timer->stop();
//   scene->applyLayout(engine->currentPositions());

// Manual placement (Canvas):
// No engine at all. Consumer positions nodes directly.
```

### Graffodil::Sugiyama

Transplanted from PlanStan `libs/graph/src/graphlayout.cpp` (800 LOC).

Five-phase Sugiyama layered layout:
1. Connected component detection (BFS, deterministic ordering)
2. Longest-path layer assignment with vertex promotion
3. Dummy vertex insertion for multi-layer edges
4. Median-based crossing minimization (adaptive, up to 24 sweeps)
5. Brandes-Kopf compact coordinate assignment (4 extreme layouts, median)

```cpp
namespace Graffodil {

class SugiyamaLayout : public LayoutEngine {
public:
    // Layout orientation. Default is LeftToRight (matches the engine's
    // historical output). For TopToBottom / BottomToTop the layer axis
    // becomes Y and the within-layer axis becomes X; setSpacing()
    // values are re-interpreted so `horizontal` is always the output
    // X-axis gap and `vertical` is always the Y-axis gap.
    enum class Direction {
        LeftToRight, RightToLeft, TopToBottom, BottomToTop
    };

    void setDirection(Direction d);
    Direction direction() const;

    void setNodeSize(const QSizeF &size);
    void setSpacing(qreal horizontal, qreal vertical);

    // Optional: ordered list of node IDs to center-align as a visual spine.
    // In PlanStan this is the critical path from DependencyGraph.
    // Other consumers might use it for any emphasized path through the graph.
    void setSpine(const QStringList &orderedNodeIds);
    void setSpineAlignment(bool enabled);

    LayoutResult compute() override;
    bool isIterative() const override { return false; }
};

}
```

**Orientation agnosticism:** `compute()` produces positions in an internal (layer-axis, within-layer-axis) frame using direction-appropriate extents and gaps, then applies a final transform — X-axis mirror for `RightToLeft`, axis swap for `TopToBottom`, both for `BottomToTop` — to both `nodePositions` and `edgeWaypoints`. The layout is otherwise identical across directions: the same crossing-minimization result, the same component stacking, the same spine alignment. Consumers can flip orientation interactively without rebuilding the scene.

**PlanStan integration** (no regression to CPM calculations):
```cpp
m_depGraph.build();                          // CPM runs in libs/scheduling/, untouched
SugiyamaLayout layout;
layout.setNodes(scene->nodes());
layout.setEdges(scene->edges());
layout.setSpine(m_depGraph.criticalPath()); // feeds CP into layout
scene->applyLayout(layout.compute());        // same positions as today
```

### Graffodil::Force

Transplanted from Corbomite `libs/forcegraph/` (~850 LOC engine + multilevel).

Modified ForceAtlas2 with:
- Barnes-Hut quadtree approximation for O(n log n) repulsion
- Adaptive speed control (per-node swinging/traction)
- Energy-based convergence detection
- Multilevel optimization (Walshaw coarsening for initial positions)

```cpp
namespace Graffodil {

class ForceLayout : public LayoutEngine {
public:
    void setCenterForce(qreal f);      // 0.0–0.05, default 0.01
    void setRepelForce(qreal f);       // 0–5000, default 1500
    void setLinkForce(qreal f);        // 0.0–0.2, default 0.05
    void setLinkDistance(qreal d);     // 20–300, default 100

    bool tick() override;
    bool isIterative() const override { return true; }
    bool isConverged() const;

    // Full simulation to convergence (uses multilevel optimization)
    LayoutResult compute() override;

    // Pin a node in place (e.g., user dragged it during simulation)
    void pinNode(const QString &id, const QPointF &pos);
    void unpinNode(const QString &id);
};

}
```

### Graffodil::Circular

New, simple layout for small graphs (sync topology use case).

```cpp
namespace Graffodil {

class CircularLayout : public LayoutEngine {
public:
    void setRadius(qreal r);
    void setCenterNode(const QString &id);  // optional: place one node at center
    LayoutResult compute() override;
    bool isIterative() const override { return false; }
};

}
```

---

## Batch Rendering Module

Optional `Graffodil::Batch` for high-node-count scenes. Transplanted from Corbomite's ForceGraph batch rendering pipeline.

```cpp
namespace Graffodil {

class BatchRenderer : public QObject {
    Q_OBJECT
public:
    explicit BatchRenderer(GraphScene *scene);

    // Switch between batch mode (single-paint-call, no per-item interaction)
    // and individual mode (standard QGraphicsItems, full interaction).
    // Consumer decides when: e.g., batch during simulation, individual when stable.
    void setBatchMode(bool enabled);
    bool isBatchMode() const;

    // Convenience: auto-switch at threshold. Set 0 to disable.
    void setAutoThreshold(int nodeCount);

    // Rebuild batch data from current scene state.
    // Call after applyLayout() or node moves.
    void synchronize();

    // LOD thresholds for progressive detail reduction at zoom levels.
    void setLodThresholds(qreal hideEdges, qreal hideLabels, qreal hideNodes);
};

// Independently usable spatial acceleration structures.

// Fixed-grid spatial index for viewport culling. Header-only, zero dependencies.
class SpatialGrid {
public:
    void rebuild(const QVector<QPointF> &positions, const QRectF &bounds);
    QVector<int> query(const QRectF &viewport) const;
};

// Barnes-Hut quadtree. Used by Force layout for O(n log n) repulsion.
// Also usable for any spatial mass/charge simulation.
class QuadTree {
public:
    void build(const QVector<QPointF> &positions, const QVector<qreal> &masses);
    QPointF computeRepulsion(const QPointF &pos, qreal mass, qreal theta) const;
};

}
```

### Rich-content node caching

Nodes whose `paint()` is expensive (markdown, thumbnails, composed glyphs) can opt into batch-mode compositing by additionally implementing `IRenderableCache`:

```cpp
class MyCardNode : public QGraphicsItem,
                   public Graffodil::IGraphNode,
                   public Graffodil::IRenderableCache {
public:
    QPixmap cachedPixmap(const QSize &) override { return m_cache; }
    QSize contentSize() const override { return m_logicalSize; }
    void invalidateCache() override { m_cache = {}; }
    // ... IGraphNode + paint() as usual
private:
    QPixmap m_cache;   // rebuilt when content changes
    QSize m_logicalSize;
};
```

When Batch mode is active, `BatchRenderer::synchronize()` queries every node via `dynamic_cast<IRenderableCache*>` and composites the returned pixmap in place of the default ellipse. Nodes that don't implement the interface fall through to the default path unchanged — this is purely additive.

---

## Utility Widgets (in Core)

### GraphMinimap

Transplanted from PlanStan `libs/graph/src/graphminimap.cpp` (147 LOC).

Small overlay rendering a scaled scene thumbnail with a draggable viewport rectangle.

```cpp
namespace Graffodil {

class GraphMinimap : public QWidget {
    Q_OBJECT
public:
    explicit GraphMinimap(QGraphicsView *parent);
    void setEnabled(bool visible);
    // Renders automatically from scene changes.
};

}
```

### AnchorHighlight

Visual feedback during edge creation — shows available connection points when hovering near a node.

```cpp
namespace Graffodil {

class AnchorHighlight : public QGraphicsItem {
public:
    void showAnchors(IGraphNode *node);
    void hideAnchors();
    void setActiveAnchor(Side side);
};

}
```

### ConnectorItem

Transplanted from PlanStan `libs/graph/src/connectoritem.cpp` (36 LOC). Small circle drawn at edge waypoints/junctions.

```cpp
namespace Graffodil {

class ConnectorItem : public QGraphicsEllipseItem {
public:
    ConnectorItem(const QPointF &pos, qreal radius = 4.5);
};

}
```

---

## Directory Structure

```
Graffodil/
├── CMakeLists.txt
├── src/
│   ├── core/
│   │   ├── CMakeLists.txt                  # Graffodil::Core
│   │   ├── include/graffodil/
│   │   │   ├── IGraphNode.h
│   │   │   ├── IGraphEdge.h
│   │   │   ├── GraphScene.h
│   │   │   ├── GraphTool.h
│   │   │   ├── GraphEdgeItem.h
│   │   │   ├── EdgePathStrategy.h
│   │   │   ├── TerminusStyle.h
│   │   │   ├── LayoutResult.h
│   │   │   ├── GraphMinimap.h
│   │   │   ├── AnchorHighlight.h
│   │   │   ├── ConnectorItem.h
│   │   │   ├── GroupStyle.h
│   │   │   └── GroupItem.h
│   │   └── src/
│   ├── sugiyama/
│   │   ├── CMakeLists.txt                  # Graffodil::Sugiyama
│   │   ├── include/graffodil/
│   │   │   └── SugiyamaLayout.h
│   │   └── src/
│   ├── force/
│   │   ├── CMakeLists.txt                  # Graffodil::Force
│   │   ├── include/graffodil/
│   │   │   ├── ForceLayout.h
│   │   │   └── MultilevelLayout.h
│   │   └── src/
│   ├── circular/
│   │   ├── CMakeLists.txt                  # Graffodil::Circular
│   │   ├── include/graffodil/
│   │   │   └── CircularLayout.h
│   │   └── src/
│   └── batch/
│       ├── CMakeLists.txt                  # Graffodil::Batch
│       ├── include/graffodil/
│       │   ├── BatchRenderer.h
│       │   ├── SpatialGrid.h               # header-only
│       │   └── QuadTree.h
│       └── src/
└── tests/
    ├── tst_graphscene.cpp
    ├── tst_groups.cpp
    ├── tst_edgepathstrategies.cpp
    ├── tst_terminus.cpp
    ├── tst_sugiyama.cpp
    ├── tst_forcelayout.cpp
    └── tst_benchmark_batch.cpp
```

All public headers install under `graffodil/` — includes are `#include <graffodil/GraphScene.h>` regardless of component.

Supports `PROJECT_IS_TOP_LEVEL` for standalone builds and exports `Graffodil::*` alias targets for in-tree `add_subdirectory()` during development.

---

## Boundary Map

What stays where after adoption:

| Component | Location | Consumes |
|-----------|----------|----------|
| `IGraphNode`, `IGraphEdge`, `GraphScene`, tools, `GraphEdgeItem`, path strategies, terminus styles, minimap, anchor highlight, connector | **Graffodil::Core** | Qt6::Widgets |
| `QuadTree`, `SpatialGrid` | **Graffodil::Spatial** | Qt6::Core |
| `SugiyamaLayout` (spine alignment) | **Graffodil::Sugiyama** | Graffodil::Core |
| `ForceLayout`, `MultilevelLayout` | **Graffodil::Force** | Graffodil::Core, Graffodil::Spatial |
| `CircularLayout` | **Graffodil::Circular** | Graffodil::Core |
| `BatchRenderer` | **Graffodil::Batch** | Graffodil::Core |
| `DependencyGraph` (CPM engine) | **PlanStan `libs/scheduling/`** | unchanged |
| `GraphNodeItem`, `PlanningColorScheme`, `RelationEditorWidget` | **PlanStan `libs/graph/`** | Graffodil::Core + Sugiyama |
| `GanttArrowRouter`, `GanttArrowItem` | **PlanStan `libs/gantt/`** | arrowhead math only |
| Sync topology widget | **PlanStan** (new) | Graffodil::Core + Circular |
| `ForceGraphNode`, `GraphDataBuilder`, `GraphViewTab` | **Corbomite** | Graffodil::Core + Force + Batch |
| `TextCardItem`, `FileCardItem`, `CanvasDocument` | **Corbomite** | Graffodil::Core |

---

## Testing Strategy

- **Unit tests per module:** Scene registry operations, edge path computation (verify QPainterPath geometry), terminus rendering (verify inset points), tool event sequences
- **Layout tests:** Transplant existing test suites from PlanStan (`tst_dependencygraph.cpp` layout portions) and Corbomite (`tst_forcelayout.cpp`, `tst_quadtree.cpp`, `tst_benchmark_layout.cpp`)
- **Integration tests:** Build a minimal graph (5 nodes, 5 edges), apply each layout engine, verify no overlapping nodes and all edges connect to correct anchors
- **Benchmark tests:** Batch rendering with 10K nodes (transplant from Corbomite's benchmarks), Sugiyama with 500-node graphs

---

## Migration Path

The library is built and validated independently, then existing code is migrated incrementally:

1. **Build Graffodil** with transplanted implementations, passing its own test suite
2. **Sync topology widget** — build from scratch on Graffodil (proves the framework on a greenfield consumer)
3. **Corbomite Canvas** — migrate to Graffodil::Core (lightest existing consumer, already well-isolated)
4. **Corbomite ForceGraph** — migrate to Graffodil::Core + Force + Batch (largest migration, validates performance)
5. **PlanStan PERT graph** — migrate to Graffodil::Core + Sugiyama (most complex consumer, validates layout fidelity)
6. **PlanStan Gantt** — adopt arrowhead utilities only (minimal change)
