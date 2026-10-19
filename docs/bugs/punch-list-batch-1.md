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

## Deliberately not in this batch

- `CreateEdgeTool` leaking its preview line: real, but only observable under a
  leak checker; it needs an ASAN run rather than a plain test.
- `setEdgesAboveNodes()` z=0 ambiguity: already accepted and documented.
- `QuadTree::insert` holding a node reference across `subdivide()`: safe as
  written (the reference is not read afterwards).
- `countCrossings` O(E²), `SelectMoveTool` guide-overlay churn,
  `computeRepulsion` region sizing: performance work, not hygiene — they belong
  with the M14/M15/G-punch performance items.
