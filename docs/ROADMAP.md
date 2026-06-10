# Graffodil Roadmap & Task Status

**Updated:** 2026-06-10
**Findings source:** [`docs/evaluation-2026-06-09.md`](evaluation-2026-06-09.md) (finding IDs B1–B6, A1–A9, P1–P5 referenced below)

This is the **single source of truth for work status** on Graffodil. When you
complete a task, tick its checkbox in the plan file AND update the phase status
here. When you finish a phase, move its row to ☑ and note the commit hash.

## For a fresh agent: how to start

1. Read `CLAUDE.md` (build/test commands), then this file.
2. Pick the first phase below that is not ☑ — phases are sequenced to avoid
   merge conflicts (later phases edit code the earlier ones restructure).
3. Open the linked plan and execute it with
   `superpowers:subagent-driven-development` or `superpowers:executing-plans`.
   Plans are bite-sized TDD tasks with exact code; specs (Phase 6) need a plan
   written first (`superpowers:writing-plans`), ideally after a brainstorm pass.
4. Build dir is `build/`. Always finish with `cmake --build build && (cd build && ctest --output-on-failure)`.

## Phase status

| Phase | Status | Doc | Findings addressed |
|-------|--------|-----|--------------------|
| 1. Bugfix punch list | ☑ complete | [plan](superpowers/plans/2026-06-09-bugfix-punchlist.md) | B1 B2 B3 B4 B5 B6 |
| 2. Edge contract | ☑ complete | [plan](superpowers/plans/2026-06-09-edge-contract.md) | A1 A2 A8 |
| 3. Scene scalability | ☑ complete | [plan](superpowers/plans/2026-06-09-scene-scalability.md) | A4 A5 |
| 4. Hot-path caching | ☑ complete | [plan](superpowers/plans/2026-06-09-hot-path-caching.md) | P1 P2 P3 |
| 5. Spatial module | ☑ complete | [plan](superpowers/plans/2026-06-09-spatial-module.md) | A3 |
| 6a. Animated layout transitions | ☐ spec'd, needs plan | [spec](specs/animated-layout-transitions.md) | feature |
| 6b. Sugiyama refactor + junction dots | ☐ spec'd, needs plan | [spec](specs/sugiyama-refactor-and-junctions.md) | A6 + ConnectorItem revival |
| 6c. Edge labels | ☐ spec'd, needs plan | [spec](specs/edge-labels.md) | feature |
| 6d. CMake install/export | ☐ spec'd, needs plan | [spec](specs/cmake-install-export.md) | A9 |
| 6e. Obstacle-aware routing | ☐ spec'd, needs plan | [spec](specs/obstacle-aware-routing.md) | feature |
| 7. Deferred / backlog | — | see below | A7 P4 P5 |

## Sequencing rationale

- **Phase 1 first** — small independent fixes; safe warm-up; clears all known bugs.
- **Phase 2 before 3** — both edit `GraphScene::adjustEdgesForNode`/`adjustAllEdges`;
  Phase 3's adjacency index dispatches to the `endpointsChanged()` virtual that
  Phase 2 introduces.
- **Phase 3 before 4** — Phase 4 measures/touches edge adjustment paths that
  Phase 3 reroutes.
- **Phase 5 any time** — pure build-system move; sequenced after 4 only to keep
  one structural change in flight at a time. Must land **before 6e** (obstacle
  routing consumes `Graffodil::Spatial`).
- **Phase 6 order:** 6a/6b/6c are independent of each other. 6a depends on
  Phases 2+3 (waypoint delivery, adjacency index). 6d should land **before the
  consumer migrations** begin. 6e depends on Phase 5.
- **Consumer migrations** (Corbomite Canvas/ForceGraph, PlanStan PERT/Gantt) are
  tracked in those repos; do them after Phases 1–5 + 6d so the API they bind to
  is final. Phase 2 contains the only breaking-ish API change (deprecations).

## Phase 7 — deferred / backlog (no spec yet; write one before starting)

- **Ownership hardening (A7):** debug-mode dangling-pointer guards — a
  `QPointer`-shadow registry asserting on stale `IGraphNode*`/`IGraphEdge*`
  access; plus an ownership section in `graffodil-design.md`.
- **Anchor spatial index (P4):** only if profiling shows `isNearAnchor()` hover
  cost on large canvases; reuse `Graffodil::Spatial`.
- **Anchor allocation churn (P5):** revisit `anchors()` value-return if profiles
  point here.
- **Qt Quick rendering backend** for 100K+ nodes (long-term; from
  `corbomite-integration-feedback.md`).
- **Z-default ambiguity (B6, second half):** consumer-set z of exactly 0.0/−1.0
  is indistinguishable from "default" in `setEdgesAboveNodes()`. Accepted for
  now; document in design doc if it ever bites a consumer.

## Status log

- **2026-06-10** — Phase 5 (spatial module) complete. QuadTree+SpatialGrid extracted into Graffodil::Spatial; Force links Spatial not Batch. Final commit: 477c0a2.
- **2026-06-10** — Phase 4 (hot-path caching) complete. GraphEdgeItem path metrics + shape cached; ForceLayout mass-by-index vector; fixed nondeterministic BFS placement. Final commit: fc6d8c1.
- **2026-06-10** — Phase 3 (scene scalability) complete. Adjacency index (O(degree) lookups) + coalesced group refresh + refreshGroupsNow(). Final commit: 82a655d.
- **2026-06-10** — Phase 2 (edge contract) complete. IGraphEdge::endpointsChanged, setWaypoints; applyLayout delivers waypoints; GraphEdgeItem strategy ownership moved to unique_ptr. Final commit: eca40f3.
- **2026-06-10** — Phase 1 (bugfix punch list) complete. All 6 bugs fixed (B1–B6). Final commit: 01141bb.
- **2026-06-09** — Full library evaluation performed; findings recorded in
  `docs/evaluation-2026-06-09.md`; roadmap, 5 plans, and 5 specs written.
  No remediation code changes yet.
