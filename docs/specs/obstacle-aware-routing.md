# Spec: Obstacle-Aware Edge Routing

**Status:** spec'd, needs implementation plan (and a brainstorm pass on the algorithm choice)
**Depends on:** Phase 5 (`Graffodil::Spatial`), Phase 2 (edge contract)
**Module:** new `Graffodil::Routing` (depends: Core, Spatial)

## Problem

All shipped `EdgePathStrategy` implementations route straight through
intervening nodes. Dense graphs (Corbomite canvas, PlanStan PERT) get edges
crossing node bodies, which reads as broken. Consumers asked for routing that
avoids node rectangles (`docs/corbomite-integration-feedback.md` triaged this
as an extension-point use; we now want a first-party implementation).

## Goals

- A drop-in `EdgePathStrategy` producing orthogonal paths that avoid all
  registered nodes' bounding rects (inflated by a clearance margin), except
  the two endpoint nodes.
- Graceful degradation: if no route is found (or the graph is degenerate —
  overlapping nodes, endpoint inside an obstacle), fall back to plain
  `OrthogonalPathStrategy` output rather than failing.
- Fast enough for interactive re-route: target ≤1 ms/edge at 100 nodes,
  with an explicit snapshot-reuse mechanism for `adjustAllEdges()` bursts.

## Non-goals

- Edge bundling, channel routing, or nudging parallel edges apart (later).
- Avoiding *edges* (only nodes are obstacles).
- Routing for the Batch renderer's straight-line edges.

## Design

### Module & dependency shape

A new module keeps Core lean and avoids Core→Spatial coupling:

```
src/routing/
  include/graffodil/ObstacleRouter.h        // the strategy + snapshot type
  src/ObstacleRouter.cpp
```
`Graffodil::Routing` links PUBLIC `Graffodil::Core`, PRIVATE `Graffodil::Spatial`.

### API

```cpp
/// Snapshot of obstacle geometry, shareable across many computePath calls.
class ObstacleSnapshot {
public:
    /// Collect nodeBoundingRect() of every node in the scene, inflated by
    /// `clearance`. Cheap to rebuild; build once per adjust pass.
    static ObstacleSnapshot fromScene(const GraphScene *scene, qreal clearance = 12.0);
    // (internals: rects + a SpatialGrid for fast "which obstacles near this segment" queries)
};

class ObstacleRoutingStrategy : public EdgePathStrategy {
public:
    /// The strategy holds a non-owning scene pointer. By default it rebuilds
    /// its snapshot lazily, at most once per scene->changed generation; call
    /// setSnapshot() to inject a shared one for a batch pass.
    explicit ObstacleRoutingStrategy(GraphScene *scene, qreal clearance = 12.0);

    void setSnapshot(std::shared_ptr<const ObstacleSnapshot> snap); // optional
    QPainterPath computePath(const Anchor &source, const Anchor &target,
                             const QList<QPointF> &waypoints = {}) override;
};
```

`computePath` ignores layout waypoints (it computes its own route); document
this loudly — Sugiyama + obstacle routing don't compose, pick one.

The endpoint-exclusion problem (the path must be allowed to start/end on the
two endpoint nodes' boundaries): exclude any obstacle rect that contains the
source/target anchor position. This avoids needing node identity inside the
strategy, keeping the `EdgePathStrategy` signature unchanged.

### Algorithm (recommendation — confirm in brainstorm before planning)

Orthogonal visibility-graph routing:
1. Candidate coordinates: x/y of both anchors plus the x/y extents of every
   inflated obstacle rect near the source-target bounding box (query via
   SpatialGrid) — a sparse Hanan grid.
2. A* over grid intersections; neighbors connected when the segment between
   them doesn't cross an obstacle (SpatialGrid segment query). Cost =
   manhattan length + bend penalty (prefer fewer bends).
3. Post-pass: collapse collinear runs; exit each anchor along its
   `outwardDirection` for the first `clearance` px (same convention as
   `OrthogonalPathStrategy`).

Alternative considered: uniform-grid A* (simpler, but resolution-dependent
and uglier paths). Decide during brainstorm; the API above is agnostic.

### Performance

- Snapshot construction: O(N) + grid fill — run once per adjust pass, not per
  edge. `GraphScene::adjustAllEdges()` stays unaware; the demo/consumer wires
  `setSnapshot()` or relies on the lazy per-generation rebuild.
- Bound the search region to the source-target bbox inflated by ~2×clearance
  + the obstacles intersecting it; bail to fallback after a node-expansion
  budget (e.g. 10k) so pathological scenes stay interactive.

## Testing

- Three nodes in a row, edge between the outer two: resulting path's stroke
  (widened by clearance/2) does not intersect the middle node's rect.
- No obstacles: output ≈ OrthogonalPathStrategy (same endpoints, ≤ same
  length + tolerance).
- Endpoint inside another node's inflated rect (overlap): falls back, no hang.
- Snapshot reuse: 100-edge adjust pass with one snapshot ≪ per-edge rebuild
  (assert call count via a test seam, not wall-clock).
- Bend penalty: straight corridor available ⇒ ≤2 bends.
