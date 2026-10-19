# Edge endpoints outlive their nodes (dangling `IGraphNode *`, ghost edge)

**Severity:** High — dangling endpoint dereference on the next drag frame, plus a
ghost edge that never invalidates. Same class as the
`AnchorHighlight` dangling-pointer crash fixed earlier; this report covers the
three remaining instances.
**Reported by:** Corbomite, 2026-10-18 (consolidated asks, ASK-M6).
**Components:** `GraphEdgeItem`, `GraphScene::removeNode`/`removeEdge`,
`CreateEdgeTool`, `SelectMoveTool`.

## Symptom

`GraphEdgeItem` holds `IGraphNode *m_source, *m_target`. `GraphScene::removeNode`
unregisters the connected edges but never clears the edges' endpoints, so an
edge object the consumer still holds keeps pointing at the node. If the consumer
then frees that node, the next `adjust()` (a drag frame, a layout pass) reads
freed memory. If it survives, `adjust()`'s early return left the last path in
place, so the edge froze at the dead node's position — a ghost with no
invalidation.

The tools had the same class of bug: `CreateEdgeTool` kept the gesture's source
as a raw pointer and dereferenced it on every mouse move; `SelectMoveTool` kept
the dragged node (and a `QHash` keyed on raw node pointers) across the whole
gesture.

## Cause

`GraphScene::removeEdge` dropped the edge from the registries and the scene but
had no way to tell the edge object its endpoints were gone. There was no
detach API on `IGraphEdge`/`GraphEdgeItem`, and `adjust()` returned silently on
a null endpoint without clearing the drawn path. The tools resolved identities
once at press time and never re-validated them.

## Fix

- `IGraphEdge::detachEndpoints()` (default no-op, so data-only edges are
  unaffected) and `GraphEdgeItem::setEndpoints(source, target)`; the scene calls
  `detachEndpoints()` in `removeEdge`, which `removeNode`'s cascade already goes
  through.
- `GraphEdgeItem::adjust()` clears the path (`prepareGeometryChange` +
  `setPath({})`) when an endpoint is missing, so the edge disappears instead of
  ghosting.
- The tools keep node **ids** and resolve them through `GraphScene::nodeForId`
  at each use (`CreateEdgeTool::sourceNode()`, `SelectMoveTool::dragNode()` /
  `liveDraggedNodes()`), because `IGraphNode` is not a `QObject` and `QPointer`
  cannot hold it. A node destroyed mid-gesture now reads as null: the edge
  gesture cancels, and the drag skips the dead participant.

## Resolution

Fixed 2026-10-19. Regression tests in `tests/tst_tools.cpp`:
`nodeRemovalDetachesConsumerHeldEdge` (fails on the parent commit — the edge
still reports the removed node and keeps its path),
`createEdgeToolSurvivesSourceNodeDestruction`, and
`selectMoveToolSurvivesDraggedNodeDestruction`. Full suite 16/16.
