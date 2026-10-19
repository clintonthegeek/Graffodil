# `~BatchRenderer` touches items the scene has already destroyed

**Severity:** High — use-after-free on every scene teardown while batch mode is
on. The virtual call path (`node->graphicsItem()->setVisible(...)`) dereferences
freed `QGraphicsItem`s and, for `IGraphNode` implementations whose graphics item
is a `QGraphicsObject`, calls a virtual function on a destroyed object.
**Reported by:** Corbomite, 2026-10-18 (consolidated asks, ASK-M5).
**Component:** `Graffodil::BatchRenderer`
(`src/batch/src/BatchRenderer.cpp`).
**Trigger:** delete a `GraphScene` that has a `BatchRenderer` child in batch
mode — the ordinary way a host tears a canvas down.

## Symptom

`SIGSEGV` raised from inside `~BatchRenderer` during `delete scene`. Reproduced
locally by `TestBatchRenderer::sceneDeletedInBatchModeDoesNotTouchDestroyedItems`:
a heap `GraphScene` with two nodes and one edge, `new BatchRenderer(scene)`,
`setBatchMode(true)`, then `delete scene`.

## Cause

`BatchRenderer` parents itself to the scene (`QObject(scene)`), so destruction
order is:

1. `~GraphScene` body,
2. the scene's members (`m_nodes`, `m_edges` — destroyed with the object),
3. `~QGraphicsScene`, which destroys every item,
4. `~QObject`, which deletes children — including this renderer.

`~BatchRenderer` called `setBatchMode(false)`, which walked
`d->scene->nodes()` / `edges()` and called `graphicsItem()->setVisible(true)`.
By step 4 all of those items are gone, and `GraphScene`'s own registries have
been destroyed too, so even reading `nodes()` is undefined.

`synchronize()` had the same problem for edges: it dereferenced
`edge->sourceNode()->graphicsItem()` with no null guard, unlike every other
call site in the library, which uses the documented-null-safe
`sourceNodeId()`.

## Fix

- Hold the scene in a `QPointer<GraphScene>` and latch a `sceneDestroyed`
  flag from the scene's `destroyed()` signal (emitted before Qt deletes its
  children), so `~BatchRenderer` only un-batches when the scene and its items
  are still alive.
- `setBatchMode()` and `synchronize()` return early when the scene is gone;
  the node/edge loops and the edge-data build tolerate null nodes, null
  endpoints and null graphics items.

## Resolution

Fixed 2026-10-19. `BatchRenderer.cpp` carries the `QPointer` + `destroyed()`
latch and the guards above; the reproduction is
`tests/tst_batchrenderer.cpp::sceneDeletedInBatchModeDoesNotTouchDestroyedItems`
(SIGSEGV on the parent commit, passes after), and the full batch suite passes
9/9.
