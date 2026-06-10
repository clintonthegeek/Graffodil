# Graffodil Library Evaluation — 2026-06-09

Full-library audit covering architecture, bugs, dead code, performance, and
development avenues. This document is the **source of record** for the findings;
the remediation work is tracked in [`docs/ROADMAP.md`](ROADMAP.md), which links
each finding to its spec/plan.

**Overall verdict:** well-designed and disciplined — clean interface-based core,
composable tools, sensible module layering, zero TODOs, good test breadth,
excellent docs. Feature-complete against `docs/graffodil-design.md`. Problems
concentrate in three areas: a leaky edge-update abstraction, missing scalability
indexes in the scene, and a handful of real bugs (two in the force simulation's
math).

---

## 1. Bugs

| # | Bug | Location | Detail |
|---|-----|----------|--------|
| B1 | ForceAtlas2 adaptive-speed cap is a no-op | `src/force/src/ForceLayout.cpp:436` | `m_globalSpeed = std::min(m_globalSpeed, m_globalSpeed * 1.5)` — `min(x, 1.5x) == x`. The FA2 rule caps growth at 1.5× the *previous* iteration's speed; as written the cap never applies, so speed can spike when swinging momentarily drops. |
| B2 | Energy convergence is backwards | `ForceLayout.cpp:482-489` | Declares convergence after energy *decreases* for 10 consecutive iterations. A correctly-cooling simulation monotonically decreases energy, so this fires while layout is still improving. Should detect a plateau (energy stops changing). The displacement fallback masks this in practice. |
| B3 | Tools never receive key-release events | `GraphScene.h:145` (missing override) | `GraphTool`/`CompositeTool` declare `keyReleaseEvent` but `GraphScene` never overrides `keyReleaseEvent` to forward it. |
| B4 | Minimap uses `itemsBoundingRect()` not `sceneRect()` | `GraphMinimap.cpp:46,116` | Documented with a specified fix in `docs/bugs/minimap-uses-itemsBoundingRect-not-sceneRect.md`; never applied. Viewport indicator escapes the minimap when panning into empty canvas. |
| B5 | Batch items claim a hardcoded ±10,000 world | `BatchNodeItem.cpp:29`, `BatchEdgeItem.cpp:34` | Graphs beyond that extent are clipped; every update dirties the whole region in the BSP index. `updatePositions()`/`updateLines()` also mutate geometry without `prepareGeometryChange()`. |
| B6 | Z-order vestige + ambiguity | `GraphScene.cpp:101-104` | Dead `prevNodeZ`/`prevEdgeZ` under `Q_UNUSED`. Also: a consumer who deliberately sets z = 0.0 / −1.0 is indistinguishable from "still at default" — `setEdgesAboveNodes()` re-stacks their items. (Vestige fixed in Phase 1; ambiguity accepted/documented.) |

## 2. Architectural problems

| # | Problem | Detail |
|---|---------|--------|
| A1 | Core depends on its own concrete edge class | `GraphScene::adjustAllEdges`/`adjustEdgesForNode` (`GraphScene.cpp:395,404`) `dynamic_cast<GraphEdgeItem*>`. Consumers implementing `IGraphEdge` directly (which the interface invites) silently get frozen edges when nodes move. **Fix:** `IGraphEdge::endpointsChanged()` virtual. |
| A2 | `applyLayout()` drops `LayoutResult::edgeWaypoints` | `Types.h:47` defines them, Sugiyama produces them, `GraphEdgeItem::setWaypoints()` exists — but `GraphScene::applyLayout` (`GraphScene.cpp:376`) only applies node positions. **Fix:** `IGraphEdge::setWaypoints()` virtual + delivery in `applyLayout`. |
| A3 | Force links Batch (a rendering module) solely for `QuadTree` | `src/force/CMakeLists.txt`. Spatial structures (`QuadTree`, dormant `SpatialGrid`) belong in their own `Graffodil::Spatial` module. |
| A4 | No adjacency index — O(E) scans with QString compares | `connectedEdges`, `edgeBetween*`, and critically `adjustEdgesForNode` walk the whole edge hash. `SelectMoveTool.cpp:180` calls it per dragged node per mouse-move ⇒ O(k·E) per frame. **Fix:** `QMultiHash<QString, IGraphEdge*>` maintained in add/removeEdge. |
| A5 | Group tracking hooks `QGraphicsScene::changed` (the firehose) | `GraphScene.cpp:281`. Fires on *every* item invalidation (hover, selection, repaints), walks all groups × members each time; connection never dropped when the last group is removed. **Fix:** coalesce to once per event-loop turn + disconnect at zero groups. |
| A6 | `SugiyamaLayout::compute()` is a ~455-line god function | `SugiyamaLayout.cpp:386`–EOF. The five documented phases aren't testable independently. **Fix:** extract phases behind a private header (golden-master tests first). |
| A7 | Ownership fragile at the edges | Scene holds raw `IGraphNode*`/`IGraphEdge*` and never deletes (documented), *except* groups (scene-owned). Nothing protects against a consumer deleting a node without `removeNode()` — registry + `GraphEdgeItem::m_source/m_target` dangle (the AnchorHighlight crash was this class of bug). Backlog: debug-mode dangling guards. |
| A8 | Mixed ownership-transfer styles in `GraphEdgeItem` | Constructor/`setPathStrategy` take raw `EdgePathStrategy*` (secretly wrapped in `unique_ptr`); `setTerminus` takes `std::unique_ptr`. **Fix:** `unique_ptr` overloads, deprecate raw. |
| A9 | No CMake install/export rules | Consumers must vendor or `add_subdirectory`. Add export targets + package config + version file before the consumer migrations. |

## 3. Performance opportunities

| # | Opportunity | Detail |
|---|-------------|--------|
| P1 | `GraphEdgeItem::paint()` recomputes path metrics per paint | `GraphEdgeItem.cpp:192-220` — `path().length()` + 4× `pointAtPercent()` per paint, each re-flattening the bezier. Cache in `adjust()`. |
| P2 | `GraphEdgeItem::shape()` builds a stroker per hit test | `GraphEdgeItem.cpp:153-162`. Cache the stroked path; invalidate in `adjust()`/`setHitWidth()`. |
| P3 | `ForceLayout::tick()` does ~6 QString-hash degree lookups per node per tick | `ForceLayout.cpp:364,370,378,384,409,418,446`. Precompute `QVector<double>` mass-by-index. |
| P4 | `CompositeTool::isNearAnchor()` walks all nodes per hover move | Already has an AABB early-reject (`CompositeTool.cpp:62-68`), so this is fine at current scale. Revisit with a spatial index only if large-canvas hover profiles point here. |
| P5 | `anchors()` returns `QList<Anchor>` by value (with `QVariant`) in hot resolve paths | Allocation churn; acceptable now, noted for the future. |

## 4. Dead / dormant code

| Item | Status | Disposition |
|------|--------|-------------|
| `ConnectorItem` | Exported, never instantiated anywhere | Give it its intended job: junction dots at Sugiyama waypoints (see spec) |
| `SpatialGrid` | Public header, used only by its own test | Move to `Graffodil::Spatial`; future consumer: obstacle routing |
| `GraphTool::keyReleaseEvent` | Unreachable via scene dispatch | Bug B3 |
| `prevNodeZ`/`prevEdgeZ` | `Q_UNUSED` vestige | Delete (B6) |
| `SugiyamaLayout::setNodeSize()`, `IRenderableCache`, `IAlignmentStrategy` | No in-repo production users | **Keep** — built for external consumers (Corbomite hooks); test implementations exist |

## 5. What's good

The Core/Extensions split with one-way dependencies is right (modulo A3). The
tool framework — `CompositeTool` matcher routing with drag capture — is genuinely
elegant. Interface granularity hits the sweet spot. The anchor system's evolution
from a compass enum to named ports with swivel resolution was the correct call.
Documentation discipline (bug reports with specified fixes, consumer-feedback
triage) is exemplary.

## 6. Spec-vs-implementation status (from docs cross-check)

- Library is feature-complete against `graffodil-design.md`; anchor API
  deliberately diverged (improved) from the spec's `Side` enum.
- All Corbomite integration hooks (IRenderableCache, IAlignmentStrategy,
  dragBegan/dragEnded, mutation-signal catalog) landed per the 2026-04-14 plan.
- Open: minimap bug (B4); consumer migrations (Corbomite Canvas/ForceGraph,
  PlanStan PERT/Gantt) not started — gated on stability window, not features.
- Long-term deferred: Qt Quick rendering backend for 100K+ nodes.
