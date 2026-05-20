# Minimap derives extents from `itemsBoundingRect()` instead of `sceneRect()`

**Severity:** Medium — visual bug; minimap is unreliable as a navigation aid but doesn't crash or block work.
**Reported by:** PlanStan, 2026-04-29 (Stage 5 graph refit user testing).
**Component:** `Graffodil::GraphMinimap` (`src/core/src/GraphMinimap.cpp`, `src/core/include/graffodil/GraphMinimap.h`).
**Affected commit (PlanStan reproduction):** `~/dev/Graffodil` HEAD as of 2026-04-29 (sibling-checkout consumer in PlanStan `master @ fd79acec`).

## Symptom

Two related visual glitches in the bird-view minimap of any
GraphScene-backed widget:

1. **Total minimap area does not represent canvas size.** The minimap's
   outer rectangle is drawn to fit `scene()->itemsBoundingRect()` plus
   a 50-px halo. Whatever the host scene authored as `sceneRect()` is
   ignored. If a downstream consumer sets an authored canvas size that
   exceeds the current item layout — or expects "the minimap shows the
   workspace" — the minimap silently disagrees.

2. **Inner viewport rectangle drifts outside the minimap on pan.** When
   the user pans the QGraphicsView past the item-bounds extent into
   empty canvas, the viewport-rect math (lines 66–73) maps real
   viewport-in-scene coordinates correctly, but those coordinates can
   fall outside the items-only base rect. Result: the cyan viewport
   rectangle slides off the minimap's edge or scales weirdly.

PlanStan reproduction: open the dependency graph view in
`build-dev/bin/PlanStan`, select a small isolated milestone scope (so
items occupy a small region), then pan the QGraphicsView far away from
that region. The minimap's blue viewport rectangle leaves the minimap
chrome.

## Cause

`GraphMinimap.cpp:46`:

```cpp
QRectF sceneRect = m_graphicsView->scene()->itemsBoundingRect();
if (sceneRect.isNull())
    return;
sceneRect.adjust(-50, -50, 50, 50);
```

…and the same idiom repeated in `mapToScene()` at line 116. The
minimap re-derives extents from items every paint, ignoring the
host's authored `sceneRect`.

This is awkward because `Graffodil::GraphScene` already exposes
`fitSceneRectToContent(margin)` (`GraphScene.cpp:167–175`) for
consumers that want canvas = items-bbox-plus-margin behavior. The
minimap should respect whatever the consumer set, not bypass it.

## Recommended fix

Single-source-of-truth on `scene()->sceneRect()`, with a fallback to
items-bbox-plus-halo when the consumer didn't author one, and a
viewport-union pass so the inner rectangle can never escape the
minimap during empty-canvas panning.

```cpp
// Replace lines 46–49 (and the symmetric block in mapToScene at 116–119):
QRectF sceneRect = m_graphicsView->scene()->sceneRect();
if (sceneRect.isEmpty()) {
    sceneRect = m_graphicsView->scene()->itemsBoundingRect();
    if (sceneRect.isNull())
        return;
    sceneRect.adjust(-50, -50, 50, 50);
}
// Union with the current viewport so the inner rectangle is always
// inside the minimap, even when panning into empty canvas:
sceneRect = sceneRect.united(
    m_graphicsView->mapToScene(m_graphicsView->viewport()->rect()).boundingRect());
```

Behavior after the fix:

- **Consumer authored sceneRect** (e.g. PlanStan after we wire
  `fitSceneRectToContent` on layout-complete): minimap reflects the
  authored canvas. ✅ resolves issue 1.
- **Consumer didn't author sceneRect**: minimap falls back to
  current behavior (items-bbox + 50 px halo). No regression.
- **Viewport panned past extents in either case**: viewport rect is
  unioned in, so the inner rectangle is guaranteed to be inside the
  minimap. ✅ resolves issue 2.

Apply the same change in both `paintEvent` (lines 46–49) and
`mapToScene` (lines 116–119) to keep click-to-navigate consistent
with the rendered region.

## Tests

Reasonable unit tests that don't require an event loop:

- Author a scene with items at (0, 0)–(100, 100), call
  `setSceneRect(0, 0, 1000, 1000)` on the scene, instantiate the
  minimap, and assert the rendered scale uses the 1000×1000 bound,
  not the 100×100 one.
- Same scene, pan the view to scene coords (5000, 5000) (i.e. far
  outside both bounds), assert the viewport rectangle in minimap-local
  coords is contained within the minimap's drawing area.
- Empty scene (no items, no sceneRect): assert paintEvent early-returns
  cleanly (current behavior, but worth pinning down).

## Out of scope for this report

- Whether the minimap should also overlay grouping regions, ego dim
  state, etc. (that's a feature ask, not a bug).
- Whether the 150×100 fixed size is right for high-DPI displays
  (separate concern; flag on a follow-up if it bites).

## Downstream notes

PlanStan currently doesn't call `fitSceneRectToContent` on its
`PlanStanGraphScene`. Once the upstream fix lands, PlanStan should
add that call after each layout cycle (and after orientation flip) so
the minimap reflects the full graph extent rather than the union of
whatever subset the viewport has touched. That is a PlanStan-side
follow-up tracked at `docs/todo/` once Graffodil ships the fix.
