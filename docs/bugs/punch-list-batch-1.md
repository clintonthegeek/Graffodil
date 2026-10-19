# Punch-list batch 1 — minimap, teardown emits, edge stroke, anchors, hit-testing

**Severity:** Medium — one user-visible gesture bug, one arithmetic hazard, and
four correctness/hygiene defects from Corbomite's consolidated asks
(2026-10-18 handoff, "smaller items" table).
**Reported by:** Corbomite, 2026-10-18.
**Fixed:** 2026-10-19.

## Items and fixes

| Item | Symptom | Fix |
|---|---|---|
| `GraphMinimap` never reset `m_dragging` | One click latched `m_dragging`; every later hover re-centred the view with no button held, because the widget had no `mouseReleaseEvent` at all. | `mouseReleaseEvent` clears the latch on a left release. Test: `tst_minimap::dragEndsOnRelease`. |
| `GraphMinimap` divided by zero-width rects | `(width()-4)/sceneRect.width()` with a degenerate scene rect scales by infinity (garbage painting; a hazard on platforms that raise on it). | Both `paintEvent` and `mapToScene` return early on a non-positive extent. Test: `tst_minimap::degenerateSceneRectPaintsSafely`. |
| `GraphScene::clearGraph()` emitted mid-teardown | `m_edges.clear()`/`m_nodes.clear()` ran *after* the emit loops, so a slot that re-entered the scene during a removal saw half-cleared registries. | Registries are cleared before the emits, and cleared edges get `detachEndpoints()`. Test: `tst_graphscene::clearGraphEmitsAfterTheRegistriesAreEmpty`. |
| `GraphEdgeItem::boundingRect()` ignored the painted stroke | Highlight and selection widen the pen in `paint()` without growing the bounding rect, so the stroke clipped and the old region was not invalidated. | The margin covers `max(hitWidth/2, paintedStroke/2 + 1)`, `setHighlighted()` calls `prepareGeometryChange()`, and a new `itemChange` invalidates geometry on selection change. Test: `tst_graphedgeitem::boundingRectCoversTheWidenedStroke`. |
| `IGraphNode::anchorToward()` returned a silent `(0,0)` | A node with no anchors routed its edges to the scene origin, which reads as a layout bug rather than a missing-anchor bug (`anchor()` already warned). | It warns once per call through `lcGraffodil`, like `anchor()`. Test: `tst_graphscene::anchorTowardWarnsWhenThereAreNoAnchors`. |
| `GraphScene::edgeActionAt()` hit-tested in item-local coordinates | `path.pointAtPercent()` returns an item-local point, but it was compared with a scene position while the sibling `nodeSubItemAt()` uses scene coordinates. A translated or rotated edge's affordance was unclickable. | The point is mapped with `mapToScene()` before the distance test. Test: `tst_graphscene::edgeActionAtUsesSceneCoordinates` (hits the translated midpoint through `edgeActionTriggered`, misses the item-local one). |

## Later additions to this batch

- `CreateEdgeTool` leaked its preview line: only `cancel()` freed it, so a tool
  destroyed mid-gesture left the item in the scene. The destructor now takes it
  back like it already did for the anchor highlight. Not observable in a plain
  test — it needs a leak checker.
- Overlapping-node hit-testing was resolved by QHash iteration order, so the
  winner changed with the hash seed (and with any Qt that randomises it).
  `nodeSubItemAt()` and `edgeActionAt()` now pick the smallest node/edge id
  deterministically; the tools' `findNodeAt()` was already deterministic because
  it walks `scene->items()` in stacking order. Test: `tst_graphscene::
  overlappingNodesHaveADeterministicWinner` varies the seed and fails on the
  parent commit (winner "b" instead of "a").
- `SugiyamaLayout`'s layer assignment (ASK-M19) was a single longest-path pass
  over a partial topological order, so a node whose predecessor had not been
  visited stayed in the same layer and its edge spanned zero layers. It now
  relaxes to a fixed point (self-loops skipped, passes capped by the node
  count). Tests: `tst_sugiyama::selfLoopDoesNotCollapseItsEdge` (fails on the
  parent commit with both nodes at x = 0), `cycleDoesNotDisturbTheChain`,
  `twoCycleKeepsTheChainLayered`.

## Deliberately not in this batch
- `setEdgesAboveNodes()` z=0 ambiguity: already accepted and documented.
- `QuadTree::insert` holding a node reference across `subdivide()`: safe as
  written (the reference is not read afterwards).
- `countCrossings` O(E²), `SelectMoveTool` guide-overlay churn,
  `computeRepulsion` region sizing: performance work, not hygiene — they belong
  with the M14/M15/G-punch performance items.
