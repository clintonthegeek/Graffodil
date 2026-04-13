# Agent Report: PlanStan Gantt Dependency Arrows

**Source:** PlanStan `libs/gantt/` (arrow parts ~300 LOC)
**Surveyed:** 2026-04-13

## Overview

PlanStan implements two Gantt view architectures:

1. **Block-based Gantt** (newer, active development) — QGraphicsView-based, uses BlockTreeModel directly
2. **Incidence-based Gantt** (older, being deprecated) — QAbstractScrollArea-based, uses KCalendarCore incidences + PlanningMetadataStore

Both use identical arrow routing and rendering logic via shared libraries in `libs/gantt/`.

## Block-Based Gantt (Primary)

**Key Files:**
- `libs/gantt/include/blockganttchartwidget.h` (133 lines)
- `libs/gantt/src/blockganttchartwidget.cpp` (608 lines)
- `src/views/blockganttviewpanel.h` (125 lines)
- `src/views/blockganttviewpanel.cpp` (794 lines)

**Architecture:**
- **View Type:** QGraphicsView
- **Scene:** Single QGraphicsScene containing GanttGridItem (z=-1), GanttBarItem per block (z=0), GanttArrowItem per dependency
- **Data Source:** BlockTreeModel + BlockStore
- **Row Sync:** Fixed height (kRowHeight = 24px), visibleRow index → y = visibleRow * 24

**Arrow Building** (lines 411-484 in blockganttchartwidget.cpp):
1. Collect all DependencyRelations from BlockDocument.relations
2. For each relation not in m_barItems → skip (invisible)
3. Build Edge vector: extract linkType, determine onCriticalPath (zero slack), determine isWishful
4. Build BarRect hash (startX, endX, centerY per visible bar)
5. Call GanttArrowRouter::computeRoutes(barRects)
6. Create GanttArrowItem per computed arrow

## Arrow Routing: GanttArrowRouter

**File:** `libs/gantt/src/ganttarrowrouter.cpp` (209 lines)
**Header:** `libs/gantt/include/ganttarrowrouter.h` (76 lines)

**Core Data Structures:**
```cpp
struct BarRect {
    double startX, endX;    // bar left/right edges
    double centerY;          // vertical center
    double height;           // bar height
};

struct Arrow {
    QVector<QPointF> points;      // polyline vertices
    bool onCriticalPath;
    bool isWishful;
    LinkType linkType;
    QRectF boundingRect;          // viewport culling
};

struct Edge {
    QString fromUid, toUid;
    LinkType type;
    bool onCriticalPath;
    bool isWishful;
};
```

**Routing Algorithm:** Stateless per call. Four dependency-type handlers with dog-leg orthogonal routing:

1. **FS (Finish-to-Start)** — from.endX → to.startX. Simple case: 6-point dog-leg. Overlap case: vertical detour.
2. **SS (Start-to-Start)** — Both from left edges, route left. 4 points.
3. **FF (Finish-to-Finish)** — Both from right edges, route right. 4 points.
4. **SF (Start-to-Finish)** — from.startX → to.endX. Symmetric to FS.

**Constants:** kGap = 6px (space from bar edge), kDetour = 12px (vertical detour distance)

**Viewport Culling:** `visibleArrows(viewport)` returns only arrows whose boundingRect intersects viewport. Bounding rect padded ±8px for arrowhead.

**Coupling:** VERY LOW. Pure geometry, no Qt signals/widgets, only geometry types.

## Arrow Rendering: GanttArrowItem

**File:** `libs/gantt/src/ganttarrowitem.cpp` (97 lines)
**Header:** `libs/gantt/include/ganttarrowitem.h` (35 lines)
**Type:** QGraphicsItem (not QGraphicsObject — lighter weight)

**Drawing:**
1. Determine pen color & style: Critical=RED 2px solid, Wishful=AMBER 1.5px dashed, Default=GRAY 1px solid
2. Draw polyline segments
3. Draw arrowhead: angle = atan2(dy, dx), two wing points at ±π/6, filled polygon

**Coupling:** VERY LOW — self-contained, depends only on QPainter and QVector<QPointF>.

## Incidence-Based Gantt (Deprecated)

- **View Type:** QAbstractScrollArea (custom painting on viewport)
- Uses same GanttArrowRouter + identical painting code
- Being deprecated in favor of block-based model

## Dependency Data Model

**Block-based (current):**
```cpp
struct DependencyRelation {
    QString from, to;
    QString type;    // "FS", "SS", "FF", "SF"
    int lag;
};
// Stored in BlockDocument.relations QVector
```

**Incidence-based (deprecated):**
```cpp
struct DependencyLink {
    QString predecessorUid;
    LinkType type;          // enum
    int lagMinutes;
};
```

## Interaction

**NO DIRECT ARROW INTERACTION** — arrows are read-only visualization. Dependencies created via tree selection:
- `BlockGanttDependencyHelper::linkBlocks()` creates consecutive FS links
- `BlockGanttDependencyHelper::unlinkBlocks()` removes links
- Uses AddRelationCommand / RemoveRelationCommand for undo

## Axis Mapping

**File:** `libs/gantt/include/ganttaxismapper.h` (114 lines)

Two modes: Schedule (fractional days) and Effort (minutes from zero).
```
pixelX = (domainOffset - visibleStart) * pixelsPerUnit + leftMarginPx
```

Zoom scales: FiveMin, Hour, Day, Week, Month, Year.

## Line Counts & Coupling Summary

| Component | Lines | Coupling |
|-----------|-------|----------|
| GanttArrowRouter | 76/209 | VERY LOW — pure algorithm |
| GanttArrowItem | 35/97 | VERY LOW — self-contained |
| BlockGanttChartWidget | 133/608 | MEDIUM — wired to BlockStore |
| GanttChartWidget (old) | 243/1632 | MEDIUM — coupled to Incidence |
| GanttAxisMapper | 114/138 | LOW — pure coordinate mapping |
| GanttBarItem | 73/174 | LOW — self-rendering |

## Reusability for Graffodil

**Directly reusable:**
- Arrowhead math (atan2 + triangle polygon) — identical to all other views
- PolylinePathStrategy concept from arrow routing

**Not reusable for Graffodil scene framework:**
- 1D constraint (time axis) makes this fundamentally different from 2D graph routing
- No per-arrow interaction (read-only)
- Router is specialized for bar-to-bar connections in a timeline context

**Graffodil integration:** PlanStan Gantt borrows arrowhead math utilities only; does not consume the scene framework.
