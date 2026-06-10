# Spec: Sugiyama Phase Extraction + Junction Dots

**Status:** spec'd, needs implementation plan
**Depends on:** nothing (refactor); junction dots benefit from Phase 2 (waypoint delivery)
**Module:** `Graffodil::Sugiyama` (+ a small Core demo/helper for junction dots)
**Addresses:** evaluation finding A6 (god function), plus the orphaned `ConnectorItem`

## Problem

`SugiyamaLayout::compute()` is one ~455-line function
(`src/sugiyama/src/SugiyamaLayout.cpp:386`–EOF). The header documents five
phases (component detection, layer assignment, dummy insertion, crossing
minimization, Brandes-Köpf coordinates) but none is independently testable —
`tst_sugiyama.cpp` can only assert end-to-end properties. Separately,
`ConnectorItem` (a small junction-dot graphics item) is exported public API
that nothing instantiates.

## Goals

- **Pure refactor first**: identical output for identical input, verified by
  golden-master tests captured *before* touching the code.
- Each phase unit-testable in isolation with meaningful invariants.
- Then: give `ConnectorItem` its intended job — optional junction dots at
  Sugiyama edge waypoints (PERT-chart style), demonstrated in the demo app.

## Non-goals

- Changing layout quality/behavior in any way during the refactor.
- New public API on `SugiyamaLayout` (direction, spine, spacing all stay).
- Scene-managed waypoint markers as core API (revisit if two consumers want it).

## Design — refactor

Private header `src/sugiyama/src/SugiyamaPhases.h` (not installed; reachable
by tests via a `target_include_directories` entry), namespace
`Graffodil::SugiyamaDetail`:

```cpp
struct InputGraph {           // index-based snapshot of nodes+edges
    int nodeCount = 0;
    QVector<QPair<int,int>> edges;        // deduped, self-loops removed
    QVector<QString> ids;                  // index -> nodeId
};
struct Layering {
    QVector<int> layerOf;                  // per node index
    int layerCount = 0;
};
struct DummyGraph {           // InputGraph + dummy vertices for long edges
    InputGraph graph;                      // dummies appended after real nodes
    int realCount = 0;                     // indices >= realCount are dummies
    QHash<int, QPair<QString,QString>> dummyOwner; // dummy -> (srcId, tgtId)
};
struct Ordering {
    QVector<QVector<int>> layers;          // per layer: node indices in order
};

QVector<QVector<int>> connectedComponents(const InputGraph &g);
Layering assignLayers(const InputGraph &g);                    // longest-path + promotion
DummyGraph insertDummies(const InputGraph &g, Layering &l);
Ordering minimizeCrossings(const DummyGraph &g, const Layering &l);  // median, ≤24 sweeps
QHash<int, QPointF> assignCoordinates(const DummyGraph &g, const Layering &l,
                                      const Ordering &o,
                                      const QSizeF &nodeSize,
                                      qreal hSpacing, qreal vSpacing); // Brandes-Köpf
```

`SugiyamaLayout::compute()` becomes: build `InputGraph` → run phases per
component → apply direction transform + spine alignment → emit
`LayoutResult` (positions + `edgeWaypoints` from dummy chains). The exact
struct shapes may flex during planning to match the existing code's
intermediate state — the requirement is *one function per phase, pure
in/out, no shared mutable state*.

### Golden-master safety net (write FIRST)

Before any extraction, add `tst_sugiyama` cases that capture full
`LayoutResult`s (positions + waypoints) for: a 10-node DAG with multi-layer
edges, a disconnected 2-component graph, a graph with the spine option set,
and each of the four directions. Assert exact equality against recorded
values. These pin behavior through the refactor; keep them afterward as
regression tests.

### Per-phase invariant tests (after extraction)

- `assignLayers`: every edge spans ≥1 layer forward; layerOf ∈ [0, layerCount).
- `insertDummies`: every edge in the output spans exactly 1 layer; dummy
  chains reconstruct the original (src, tgt) pairs.
- `minimizeCrossings`: crossing count ≤ the input ordering's crossing count
  (provide a small crossing-counter helper in the test).
- `assignCoordinates`: within a layer, consecutive nodes separated by
  ≥ nodeSize + spacing on the within-layer axis.

## Design — junction dots

Once waypoints flow (Phase 2 delivers them to edges automatically), junction
dots are a *consumer-side rendering choice*. Deliverable:

1. `ConnectorItem` gets a doc comment stating its purpose (junction dot at
   edge waypoints) and a usage example.
2. The demo app (`demo/DemoWindow.cpp`) gains a "Show junctions" toggle in its
   Sugiyama mode: after `applyLayout`, instantiate one `ConnectorItem` per
   waypoint from the `LayoutResult`, track them in a list, delete on re-layout
   or toggle-off.
3. If both PlanStan and Corbomite end up wanting scene-managed markers, then
   promote to `GraphScene::setWaypointMarkersVisible(bool)` — out of scope
   until that demand is real (YAGNI).

## Risks

- The existing code interleaves phases (e.g. spine alignment inside
  coordinate assignment). The golden masters are the contract; if a clean
  seam genuinely can't reproduce byte-identical output, stop and re-plan —
  do not relax the masters silently.
