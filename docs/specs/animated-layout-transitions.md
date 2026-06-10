# Spec: Animated Layout Transitions

**Status:** spec'd, needs implementation plan
**Depends on:** Phase 2 (edge contract — `endpointsChanged()`/`setWaypoints()` virtuals), Phase 3 (adjacency index)
**Module:** `Graffodil::Core` (`GraphScene`)

## Problem

`GraphScene::applyLayout()` teleports nodes to their new positions. When a
consumer re-runs a layout (orientation flip, node added, engine switched), the
user loses all sense of which node went where. Animated interpolation between
the old and new positions is the single most visible polish feature for every
consumer app.

## Goals

- One-call animated transition to a `LayoutResult`, with sane defaults.
- Edges follow nodes every frame; groups follow via the existing coalesced
  refresh.
- Robust against scene mutation mid-flight (node removed during animation must
  not crash).
- Interruptible: starting a new layout (animated or not) or stopping cancels
  cleanly by snapping to the *new* targets.

## Non-goals

- Animating the force layout's per-tick streaming (already incremental).
- Per-node easing/stagger choreography (one global curve is enough).
- Animating edge waypoint paths point-by-point (see Behavior).

## API

```cpp
// GraphScene, public:

/// Animate all nodes from their current positions to result.nodePositions
/// over durationMs, easing with `curve`. Edge paths re-adjust every frame;
/// edge waypoints (if any) are delivered at the final frame. Calling this
/// (or applyLayout(), or stopLayoutAnimation()) while an animation is
/// running first completes the running one by snapping to its targets.
void applyLayoutAnimated(const LayoutResult &result,
                         int durationMs = 300,
                         const QEasingCurve &curve = QEasingCurve(QEasingCurve::InOutCubic));

bool isLayoutAnimating() const;

/// Snap the running animation to its final positions immediately. No-op if
/// not animating.
void stopLayoutAnimation();

signals:
/// Emitted exactly once per applyLayoutAnimated() call, after final
/// positions (and waypoints) are applied — whether it ran to completion
/// or was interrupted.
void layoutAnimationFinished();
```

## Behavior

1. On start: capture `(nodeId, startPos, endPos)` for every entry in
   `result.nodePositions` whose node is currently registered. Nodes in the
   scene but absent from the result don't move (same as `applyLayout`).
2. Drive with a single `QVariantAnimation` (0.0 → 1.0), `this` as parent.
   Per frame: for each captured tuple, re-resolve the node by id
   (`nodeForId`) — if it has been removed mid-flight, skip it — and
   `setPos(lerp(start, end, eased_t))`. Then adjust only affected edges:
   collect the union of dragged node ids once at start and call
   `adjustEdgesForNode()` per id (cheap post-Phase-3), or `adjustAllEdges()`
   if >50% of nodes are moving.
3. Waypoints: curved/orthogonal paths interpolate badly. During flight,
   edges adjust with their *existing* waypoints; at the final frame deliver
   `result.edgeWaypoints` via the Phase-2 path (same loop as `applyLayout`)
   and do a final `adjustAllEdges()` + synchronous `refreshGroups()`.
4. Interruption: `applyLayout()`, `applyLayoutAnimated()`, and
   `stopLayoutAnimation()` all call a private `finishLayoutAnimation()`
   that jumps to t=1 semantics (final positions + waypoints + group refresh +
   `layoutAnimationFinished()`), then proceed.
5. `clearGraph()` and `removeNode()` do **not** stop the animation; the
   per-frame id re-resolution makes removed nodes drop out safely.

## Implementation notes

- Store the animation state in a small struct
  (`std::unique_ptr<LayoutAnimationState>`) on GraphScene: the captured
  tuples, the pending `LayoutResult` (for waypoints), and the
  `QVariantAnimation*`.
- Use `QVariantAnimation::valueChanged` with `this` as context object.
- Don't fight `SelectMoveTool`: if the user drags a node mid-animation, the
  next frame overwrites the drag — acceptable for v1; document it. (A later
  refinement could drop dragged ids from the tuple list on `dragBegan`.)

## Testing

QTest with event loop (`QSignalSpy` + `QTest::qWait`):
- Final positions equal `result.nodePositions` after the animation ends;
  `layoutAnimationFinished` emitted exactly once.
- `stopLayoutAnimation()` mid-flight snaps to targets immediately.
- Second `applyLayoutAnimated()` mid-flight: first one's targets applied,
  then second runs; two `layoutAnimationFinished` emissions total.
- `removeNode()` mid-flight: no crash; remaining nodes reach targets.
- Edge with waypoints: waypoints present in path only after completion.
- Zero/negative duration: behaves exactly like `applyLayout()` (still emits
  the signal).
