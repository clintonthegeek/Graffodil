# Agent Report: PlanStan PERT/Critical-Path Graph View

**Source:** PlanStan `libs/graph/` (~5,768 LOC)
**Surveyed:** 2026-04-13

## Overall Architecture

PlanStan has a well-structured, modular graph drawing system organized into:
- **Core Library**: `/home/clinton/dev/PlanStan/libs/graph/` (5,768 LOC total)
- **View Panel**: `/home/clinton/dev/PlanStan/src/views/graph/` (503 LOC)
- **Data Model**: `/home/clinton/dev/PlanStan/libs/scheduling/` (DependencyGraph, for CPM)

The graph is a PERT-style dependency network with full CPM (Critical Path Method) analysis, orthogonal and direct edge routing, and interactive tools.

## 1. Main Graph View Widget

**File**: `libs/graph/include/dependencygraphview.h` (162 LOC)
**Class**: `DependencyGraphView : public ViewInterface, public ISelectionOwner`

**Key Methods**:
```cpp
void setActiveCal(KCalendarCore::MemoryCalendar *cal) override;
void refresh() override;
void autoLayout();           // Calls GraphLayout::topologicalLayout()
void fitAll();
void zoomIn/Out/Reset();
void activateSelectTool();
void activateCreateEdgeTool();
void activatePanTool();
void setOrthogonalRouting(bool);
void setShowCriticalPath(bool);
void setShowMinimap(bool);
void setViewPalette(const ViewPalette &);
void setCalendarFilters(const QStringList &);
void setPortfolioMode(bool);  // Multi-project mode
```

**Data Members**:
```cpp
QGraphicsView *m_graphicsView;
DependencyScene *m_scene;
DependencyGraph m_depGraph;        // CPM data model
SelectMoveTool *m_selectTool;
CreateEdgeTool *m_edgeTool;
PanZoomTool *m_panTool;
GraphMinimap *m_minimap;
ViewPalette m_viewPalette;        // Theme colors
```

**Signals**:
- `nodeDoubleClicked(const IncidenceRef &)`
- `nodeContextMenuRequested(const QList<IncidenceRef> &, const QPoint &)`
- `nodeSelected(const QString &uid)`, `edgeSelected(...)` — for relation editor
- `toolChanged(ActiveTool)`
- `portfolioProjectActivated(const QString &projectId)`

## 2. Scene & Rendering

**File**: `libs/graph/include/dependencyscene.h` (83 LOC)
**Class**: `DependencyScene : public QGraphicsScene`

**Key Methods**:
```cpp
GraphNodeItem *addNode(const KCalendarCore::Incidence::Ptr &, const QString &calendarId);
void removeNode(const QString &uid);
GraphNodeItem *nodeForUid(const QString &uid) const;

DependencyEdgeItem *addEdge(GraphNodeItem *source, GraphNodeItem *target);
void removeEdge(DependencyEdgeItem *edge);
DependencyEdgeItem *edgeBetween(const QString &sourceUid, const QString &targetUid) const;

void setEdgeRoutingMode(DependencyEdgeItem::RoutingMode);
void setActiveTool(GraphTool *tool);
void setAlignmentGuides(const QList<QLineF> &);  // For node movement snap-guides
void addConnector(const QPointF &, bool critical);  // Visual waypoint markers
```

## 3. Node Representation

**File**: `libs/graph/include/graphnodeitem.h` (98 LOC)
**Class**: `GraphNodeItem : public QGraphicsObject` (452 LOC implementation)

**Base Data Structure**: `GraphNodeRenderData` (45 LOC)
```cpp
struct GraphNodeRenderData {
    IncidenceRenderData base;        // Title, color, completion state
    int effortMinutes;               // Duration
    int slackMinutes;                // CPM slack
    bool onCriticalPath;
    bool infeasible;
    int earliestStartMinutes;        // CPM timing (from DependencyGraph)
    int earliestFinishMinutes;
    int latestStartMinutes;
    int latestFinishMinutes;
    int deadlineMinutes;
    bool isOverdue, isScheduled;
    int completionPercent;
    QDate deadlineDate, scheduledDate;
    bool hasUnresolvedPredecessors;  // Wishful scheduling indicator
    QPointF position;                // Persisted layout position
    QStringList blockerUids;
    QStringList dependentUids;
};
```

**Node Size**: 180x100 pixels (configurable)

**Anchor System** (4 cardinal points for edge connection):
```cpp
enum Anchor { North, South, East, West };
QPointF anchorPoint(Anchor a) const;
QPointF portPoint(Anchor side, const DependencyEdgeItem *forEdge) const;  // Multi-port support
```

**Rendering** (multi-level detail with LOD):
1. **LOD < 0.3** (zoomed out): Colored dot only
2. **0.3 <= LOD < 0.6** (medium zoom): Title only
3. **LOD >= 0.6** (detail zoom): Full node with effort badge, slack pip, CPM timing, deadline indicator, completion bar, scheduling state

**Visual Indicators**:
- Critical path: outer glow effect (3px colored border)
- Hover: shows anchor points; cursor changes to "open hand"
- Selection: standard QGraphicsItem selection highlighting
- Node chrome: rounded rectangle with border based on status

## 4. Edge/Arrow Representation

**File**: `libs/graph/include/dependencyedgeitem.h` (82 LOC)
**Class**: `DependencyEdgeItem : public QGraphicsPathItem` (432 LOC implementation)

**Routing Modes**:
```cpp
enum RoutingMode { Direct, Orthogonal };
```

**Direct Mode**: Straight line from source to target anchor.

**Orthogonal Mode**: Right-angle segments with waypoints:
- Classifies edge direction (left-to-right, right-to-left, same-column)
- Routes through "channels" on node sides to minimize overlaps
- Supports waypoints from layout algorithm for multi-layer edges

**Link Types** (CPM semantic):
```cpp
enum LinkType { FS, SS, FF, SF };  // Finish-to-Start, Start-to-Start, etc.
int lagMinutes;                      // Delay between tasks
```

**Path Computation**:
```cpp
void computeDirectPath();
void computeOrthogonalPath();
void computeSimpleOrthogonalPath();
void computeWaypointOrthogonalPath();
void drawArrowHead(QPainter *, const QPointF &tip, const QPointF &from);
```

## 5. Layout Algorithm

**File**: `libs/graph/src/graphlayout.cpp` (800 LOC)
**Namespace**: `GraphLayout`

**Algorithm**: **Sugiyama Layered Layout** with critical-path spine alignment.

**5 Phases**:

1. **Component Detection** — BFS for connected components, deterministic ordering via WBS index
2. **Longest-Path Layer Assignment** — Topological order: assign node to layer = max(predecessors) + 1, vertex promotion with 2-10 iterations
3. **Dummy Vertex Insertion** — For edges spanning >1 layer, insert dummy nodes. Critical-path spine seeding identifies one real CP node per layer
4. **Median-Based Crossing Minimization** — Adaptive median-of-neighbors, forward/backward sweeps, early stop at 3 consecutive non-improving or max 24 sweeps
5. **Brandes-Kopf Compact Coordinate Assignment** — 4 extreme layouts (forward/backward x top/bottom), median balances all directions. Post-pass: CP center-spine alignment

**Input/Output**:
```cpp
LayoutResult topologicalLayout(
    const DependencyGraph &graph,
    const QSizeF &nodeSize,
    qreal horizontalSpacing = 60,
    qreal verticalSpacing = 40);

struct LayoutResult {
    QHash<QString, QPointF> positions;
    QHash<QPair<QString,QString>, QList<QPointF>> edgeWaypoints;
};
```

## 6. Interaction Tools

All inherit from `GraphTool` base (31 LOC):
```cpp
class GraphTool : public QObject {
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *) = 0;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *) = 0;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *) = 0;
    virtual void keyPressEvent(QKeyEvent *);
    virtual void activate();
    virtual void deactivate();
};
```

- **SelectMoveTool** (158 LOC): Click select, drag move with alignment guides, rubber-band, shift+click toggle, delete key removes edges
- **CreateEdgeTool** (158 LOC): Click source anchor, dashed preview, click target creates edge, escape cancels, prevents self-loops and duplicates
- **PanZoomTool** (103 LOC): Left-drag pan, middle-drag zoom to cursor

## 7. Color & Theming

**File**: `libs/graph/include/planningcolorscheme.h` (42 LOC)

Shared semantic colors for Gantt + Graph views:
```cpp
namespace PlanningColorScheme {
    QColor nodeBackground(const ViewPalette &vp, const GraphNodeRenderData &data);
    QColor nodeFrame(const ViewPalette &vp, const GraphNodeRenderData &data);
    QColor criticalPathEdge(const ViewPalette &);
    QColor normalEdge(const ViewPalette &);
    QColor slackHighlight(const ViewPalette &, int slackMinutes);
};
```

## 8. Minimap & Navigation

**File**: `libs/graph/include/graphminimap.h` (41 LOC)

Small overlay widget (bottom-right corner): renders entire scene scaled to ~150x100px, viewport rectangle draggable to pan main view.

## 9. View Panel Integration

**File**: `src/views/graph/dependencygraphviewpanel.h` (80 LOC)
**Class**: `DependencyGraphViewPanel : public ViewPanelBase` (423 LOC)

MDI-layer glue: creates/owns DependencyGraphView, toolbar actions, project switching, temporal range filters, forwards selection signals to RelationEditorWidget.

## 10. Relation Editor Widget

**File**: `libs/graph/include/relationeditorwidget.h` (87 LOC)

Compact editor for dependency relations: shows predecessors/successors, edits link type (FS/SS/FF/SF) and lag, add/remove with undo, adapts to narrow/wide layout.

## 11. Data Model: DependencyGraph

**Location**: `libs/scheduling/include/dependencygraph.h`

Pure computation engine (no Qt models):
- `addTask(uid, parentUid, effortMinutes, scheduled, deadline, ...)`
- `addExplicitDependency(fromUid, toUid, linkType, lag)`
- `setParentOrdered(parentUid, bool)`
- `build()` → computes all timings, critical path
- Queries: `topologicalOrder()`, `criticalPath()`, `timingInfo(uid)`, `allSuccessors(uid)`

## 12. File Manifest

### Core Graph Library (`libs/graph/`)
| File | Lines | Purpose |
|------|-------|---------|
| `include/dependencygraphview.h` | 162 | Main view widget |
| `src/dependencygraphview.cpp` | 1164 | View implementation |
| `include/dependencyscene.h` | 83 | Scene manager |
| `src/dependencyscene.cpp` | 256 | Scene implementation |
| `include/graphnodeitem.h` | 98 | Node rendering |
| `src/graphnodeitem.cpp` | 452 | Node implementation |
| `include/dependencyedgeitem.h` | 82 | Edge rendering |
| `src/dependencyedgeitem.cpp` | 432 | Edge implementation |
| `include/graphtool.h` | 31 | Tool base class |
| `include/selectmovetool.h` | 47 | Selection/move tool |
| `src/selectmovetool.cpp` | 158 | Tool implementation |
| `include/createedgetool.h` | 54 | Edge creation tool |
| `src/createedgetool.cpp` | 158 | Tool implementation |
| `include/panzoomtool.h` | 40 | Pan/zoom tool |
| `src/panzoomtool.cpp` | 103 | Tool implementation |
| `include/graphlayout.h` | 49 | Layout algorithm |
| `src/graphlayout.cpp` | 800 | Sugiyama + CP-spine layout |
| `include/graphminimap.h` | 41 | Minimap widget |
| `src/graphminimap.cpp` | 147 | Minimap implementation |
| `include/graphnoderenderdata.h` | 45 | Node data struct |
| `include/connectoritem.h` | 39 | Waypoint marker |
| `src/connectoritem.cpp` | 36 | Connector implementation |
| `include/planningcolorscheme.h` | 42 | Color scheme |
| `src/planningcolorscheme.cpp` | 60 | Color implementation |
| `include/relationeditorwidget.h` | 87 | Relation editor |
| `src/relationeditorwidget.cpp` | 523 | Editor implementation |
| `include/graphcommands.h` | 36 | Undo/redo commands |
| `src/graphcommands.cpp` | 40 | Command stubs |

## 13. Reusability Assessment

### Highly Reusable (Generic Graph Components)
1. **GraphTool base class** — extensible tool framework, no PERT-specific logic
2. **DependencyScene** — generic QGraphicsScene with node/edge management
3. **GraphNodeItem** — custom node rendering (LOD, anchors) but could abstract data binding
4. **DependencyEdgeItem** — routing logic (Direct/Orthogonal) is domain-agnostic; only CPM coloring ties to planning
5. **GraphLayout** — topological layout algorithm is pure graph theory; spine alignment is the only PERT-specific tweak
6. **GraphMinimap** — standalone minimap widget for any QGraphicsView
7. **ConnectorItem** — simple visual connector, domain-independent

### Tightly Coupled to PERT/Planning Semantics
1. **DependencyGraphView** — requires DependencyGraph, PlanningMetadataStore, CPM color scheme
2. **GraphNodeRenderData** — includes CPM fields (slack, critical path, infeasible flag)
3. **PlanningColorScheme** — hardcoded to planning semantics
4. **RelationEditorWidget** — edited for FS/SS/FF/SF link types and lag
5. **Phase 3b & 5b in GraphLayout** — critical-path spine alignment specific to PERT

## 14. Key Design Patterns

1. **Tool Pattern** (GraphTool + subclasses) — cleanly separates interaction modes
2. **Scene Observer** (DependencyScene dispatches to active tool) — decouples tools from view
3. **Render Data** (GraphNodeRenderData, IncidenceRenderData) — separates data from rendering
4. **Color Scheme Strategy** (PlanningColorScheme namespace) — reusable across views
5. **Adapter** (DependencyGraphViewPanel) — integrates generic graph into MDI framework
6. **Command Pattern** (GraphCommands, undo/redo) — stubs present, ready for extension
