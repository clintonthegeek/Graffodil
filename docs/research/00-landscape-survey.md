# Graph Drawing Landscape Survey

Research conducted 2026-04-13 across PlanStan and Corbomite codebases.

## Five Graph Drawing Contexts

### 1. PlanStan PERT/CPM Graph View (`libs/graph/`, ~5,768 LOC)

**Purpose:** Quasi-PERT dependency network emphasizing critical path spine.

| Aspect | Detail |
|--------|--------|
| **View** | QGraphicsView/QGraphicsScene |
| **Layout** | Sugiyama layered (5-phase: component detection, longest-path layers, dummy vertices, median crossing minimization, Brandes-Kopf coordinates) + critical-path spine alignment |
| **Nodes** | `GraphNodeItem` (QGraphicsObject), 180x100px, 3-tier LOD rendering, 4-anchor system (N/S/E/W) with multi-port support |
| **Edges** | `DependencyEdgeItem` (QGraphicsPathItem), Direct + Orthogonal routing modes, orthogonal uses waypoints from layout |
| **Data** | `DependencyGraph` (CPM engine in `libs/scheduling/`), FS/SS/FF/SF link types + lag |
| **Tools** | `GraphTool` base class -> `SelectMoveTool`, `CreateEdgeTool`, `PanZoomTool` |
| **Extras** | Minimap, ConnectorItems at waypoints, RelationEditorWidget, PlanningColorScheme |
| **Interaction** | Click-select, drag-move with alignment guides, rubber-band, drag-to-create-edge with anchor snapping |

**Key files:**
- `libs/graph/include/dependencygraphview.h` (162 LOC) — main view
- `libs/graph/src/graphlayout.cpp` (800 LOC) — Sugiyama layout
- `libs/graph/include/graphtool.h` (31 LOC) — tool base class
- `libs/graph/src/dependencyedgeitem.cpp` (432 LOC) — edge routing
- `libs/graph/src/graphnodeitem.cpp` (452 LOC) — node rendering

---

### 2. PlanStan Gantt Dependency Arrows (`libs/gantt/`, arrow parts ~300 LOC)

**Purpose:** 1D orthogonal dependency arrows between timeline bars.

| Aspect | Detail |
|--------|--------|
| **View** | QGraphicsView (block-based Gantt) |
| **Layout** | N/A — arrows route between fixed bar positions |
| **Routing** | `GanttArrowRouter` (209 LOC) — dog-leg orthogonal routing per link type (FS/SS/FF/SF) with overlap detours |
| **Rendering** | `GanttArrowItem` (97 LOC) — QGraphicsItem, polyline + triangular arrowhead |
| **Constraint** | 1D: X = time axis, Y = row index. Arrows route horizontally along time, vertically between rows |
| **Interaction** | Read-only visualization. No arrow click/drag. Dependencies created via tree selection helpers |
| **Styling** | Critical path = red 2px, Wishful = amber dashed, Normal = gray 1px |

**Key files:**
- `libs/gantt/include/ganttarrowrouter.h` (76 LOC) — pure-geometry router
- `libs/gantt/src/ganttarrowrouter.cpp` (209 LOC) — FS/SS/FF/SF routing
- `libs/gantt/include/ganttarrowitem.h` (35 LOC) — rendering item
- `libs/gantt/src/ganttarrowitem.cpp` (97 LOC) — polyline + arrowhead paint

**Coupling assessment:** VERY LOW. Router is pure geometry (no Qt widgets). ArrowItem is self-contained.

---

### 3. PlanStan Sync Topology Visualizer (NOT YET BUILT)

**Purpose:** Small graph showing sync backends as nodes, sync mappings as directed edges.

| Aspect | Detail |
|--------|--------|
| **Scale** | 3-7 nodes, ~400x300px compact widget |
| **Nodes** | Rounded rectangles: type icon + display name + status dot |
| **Edges** | Directed arrows with direction indicators (two-way, push, pull) |
| **Layout** | Force-directed or circular for small counts |
| **Interaction** | Click node -> detail panel, click edge -> edit mapping properties |
| **Data** | `SyncMapping` from `libs/core/include/synctypes.h` |

**Source:** `docs/todo/multi-backend-ux-sync-topology.md`, `docs/proposals/2026-04-02-multi-backend-ux-affordances.md`

---

### 4. Corbomite Force-Directed Graph (`libs/forcegraph/`, ~3,283 LOC)

**Purpose:** Visualize all markdown files as nodes with wiki-link connections. Optimized for thousands of nodes.

| Aspect | Detail |
|--------|--------|
| **View** | QGraphicsView/QGraphicsScene |
| **Layout** | Modified ForceAtlas2 with adaptive speed control. Multilevel optimization (Walshaw coarsening). Barnes-Hut quadtree for O(n log n) repulsion |
| **Nodes** | `ForceGraphNode` (QGraphicsEllipseItem), radius = 4 + log(1+degree)*3, 5 types (Hub/Regular/Orphan/Unresolved/DailyNote) |
| **Edges** | `ForceGraphEdge` (QGraphicsLineItem), straight lines with optional arrowheads |
| **Performance** | `BatchNodeItem`/`BatchEdgeItem` for single-paint-call rendering, `SpatialGrid` (64x64) for viewport culling, edge hiding during interaction (>2000 nodes), multi-tier LOD |
| **Interaction** | Pan, zoom, hover highlight, click-select, double-click to open file, pin node position |
| **Scale** | 10K nodes tested (8-20s layout depending on topology) |

**Key files:**
- `libs/forcegraph/include/forcegraph/ForceLayoutEngine.h` (71 LOC) — physics simulation
- `libs/forcegraph/src/ForceLayoutEngine.cpp` (~600 LOC) — ForceAtlas2 + convergence
- `libs/forcegraph/src/MultilevelLayout.cpp` (~250 LOC) — Walshaw coarsening
- `libs/forcegraph/include/forcegraph/QuadTree.h` (36 LOC) — Barnes-Hut
- `libs/forcegraph/include/forcegraph/SpatialGrid.h` (97 LOC) — viewport culling
- `libs/forcegraph/src/BatchNodeItem.cpp` / `BatchEdgeItem.cpp` — batch rendering

**Coupling assessment:** Already a standalone library. No Corbomite dependencies in `libs/forcegraph/`.

---

### 5. Corbomite Canvas (`libs/canvas/`, ~3,851 LOC)

**Purpose:** Manually arranged cards connected by curvy bezier arrows (Obsidian Canvas compatible).

| Aspect | Detail |
|--------|--------|
| **View** | QGraphicsView/QGraphicsScene |
| **Layout** | Manual only — user drags cards to position |
| **Nodes** | `TextCardItem` (302 LOC, rich markdown), `FileCardItem` (219 LOC, file reference), `GroupItem` (229 LOC, container) |
| **Edges** | `EdgeItem` (176 LOC) — QPainterPath with cubic bezier curves. Curvature = min(distance * 0.4, 80.0). Control points extend from connection side |
| **Connection** | `ConnectableItem` interface, `connectionPoint(Side)` for 4 cardinal sides |
| **Interaction** | `SelectMoveTool`, `CreateCardTool`, `CreateEdgeTool` — drag near card edge to create connection |
| **Data** | JSON Canvas 1.0 format (jsoncanvas.org). `CanvasDocument` handles serialization |
| **Undo** | Full QUndoCommand support |

**Key files:**
- `libs/canvas/src/EdgeItem.cpp` (176 LOC) — bezier curve rendering
- `libs/canvas/include/canvas/ConnectableItem.h` (20 LOC) — connection point interface
- `libs/canvas/src/CanvasTool.cpp` (606 LOC) — all tool implementations
- `libs/canvas/include/canvas/CanvasTypes.h` (84 LOC) — data structures

**Coupling assessment:** Standalone library. Markdown rendering injected via callback.

---

## Cross-Cutting Analysis

### Shared Patterns Already Present

| Pattern | PlanStan Graph | PlanStan Gantt | Corbomite ForceGraph | Corbomite Canvas |
|---------|---------------|----------------|---------------------|-----------------|
| QGraphicsView/Scene | Yes | Yes | Yes | Yes |
| Tool state machine | GraphTool base | N/A | N/A (view handles) | CanvasTool base |
| Node items | QGraphicsObject | QGraphicsObject (bars) | QGraphicsEllipseItem | QGraphicsObject |
| Edge items | QGraphicsPathItem | QGraphicsItem | QGraphicsLineItem | QGraphicsPathItem |
| LOD rendering | 3-tier | N/A | Multi-tier + batch | N/A |
| Minimap | Yes | N/A | N/A | N/A |
| Arrowheads | Triangular (atan2) | Triangular (atan2) | Optional (atan2) | Triangular (atan2) |
| Connection anchors | 4-cardinal + ports | Bar edges (L/R) | N/A (center) | 4-cardinal |
| Undo/redo | Stubs | Commands | N/A | Full commands |

### Fundamental Differences

| Dimension | Range |
|-----------|-------|
| **Scale** | 3-7 nodes (sync) to 10K+ (force graph) |
| **Layout** | Manual, Force-directed, Sugiyama layered, Circular, 1D constrained |
| **Edge style** | Straight lines, Polylines, Orthogonal routed, Cubic bezier |
| **Node complexity** | Simple circles to rich multi-field rectangles with LOD |
| **Interaction** | Read-only arrows to full create/move/resize/connect |
| **Dimensionality** | 1D constrained (Gantt) vs. 2D free (everything else) |

### Reuse Candidates (by coupling)

**Easily extractable (very low coupling):**
- Arrowhead drawing (identical atan2 + polygon across all 4 views)
- GanttArrowRouter (pure geometry)
- GanttArrowItem (self-contained polyline renderer)
- ConnectableItem interface (connection point abstraction)
- GraphTool base class (tool state machine)
- GraphMinimap (generic QGraphicsView overlay)
- SpatialGrid (header-only, zero dependencies)
- QuadTree (pure algorithm)

**Extractable with interface abstraction:**
- Edge rendering strategies (polyline, bezier, orthogonal as pluggable strategies)
- Node anchor/port system (4-cardinal generalized)
- Scene management (node/edge registry, tool dispatch)
- Batch rendering pipeline (for high-node-count scenarios)

**Domain-specific (keep in consuming code):**
- Layout algorithms (each too specialized to generalize usefully)
- Node rendering (data binding too diverse)
- Data models (DependencyGraph, GraphNode, CanvasNode, SyncMapping)
- Application integration (view panels, control panels, data builders)
