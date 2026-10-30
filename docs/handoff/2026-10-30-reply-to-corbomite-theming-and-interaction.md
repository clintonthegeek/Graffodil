# Reply to "what we would like from Graffodil next"

**From:** Graffodil maintainers
**Date:** 2026-10-30
**Re:** [`2026-10-30-from-corbomite-theming-and-interaction.md`](2026-10-30-from-corbomite-theming-and-interaction.md)

Short version: your pin (`57c8024`) already contains more than you're crediting
it for. Roughly half the smaller-rows table asks for things we shipped on
2026-10-19, one day after the measurements you cited. Please re-verify against
the tip before filing any of those rows against us a second time.

## Already fixed — re-point your pin, no code change needed

These are all in `57c8024` and were landed in `5c62d01` (punch-list batch 1)
plus `bb41c0d` (Sugiyama layer fixed-point), all with tests:

| Row | Where it's handled |
|---|---|
| `edgeActionAt` item-local coords | Fixed; `edgeActionAt` maps to scene coords before the distance test. `tst_graphscene::edgeActionAtUsesSceneCoordinates`. |
| `GraphMinimap` never resets `m_dragging` | Fixed; `mouseReleaseEvent` added. `tst_minimap::dragEndsOnRelease`. |
| `GraphMinimap` divides by zero-extent rects | Fixed; early return on non-positive scene extent. `tst_minimap::degenerateSceneRectPaintsSafely`. |
| `GraphScene::clearGraph()` emits mid-teardown | Fixed; registries emptied (and edges detached) before emits. `tst_graphscene::clearGraphEmitsAfterTheRegistriesAreEmpty`. |
| `GraphEdgeItem::boundingRect()` ignores pen width | Fixed; margin covers `max(hitWidth/2, paintedStroke/2 + 1)` and geometry changes invalidate on highlight/selection. `tst_graphedgeitem::boundingRectCoversTheWidenedStroke`. |
| `IGraphNode::anchorToward()` silent `(0,0)` | Fixed; warns once per call, matching `anchor()`. `tst_graphscene::anchorTowardWarnsWhenThereAreNoAnchors`. |
| `CreateEdgeTool` leaks its preview line | Fixed; destructor reclaims it alongside the anchor highlight. |
| Overlapping hit-test winner nondeterministic | Fixed (you flagged this row in passing); smallest-id wins, seed-varied. `tst_graphscene::overlappingNodesHaveADeterministicWinner`. |

## Real asks we're taking (accepted, not yet started)

1. **M14 — theming surface.** Agreed this is the big one and that our own
   hardcoding is the root cause. We will adapt (rather than reimplement) your
   `CanvasPalette` shape unless it fights Graffodil's data model — expect
   `Graffodil::Palette` + `GraphScene::setPalette()` + an optional per-node
   colour provider for `BatchRenderer`. When this lands you can drop the
   `QColor(0,120,215)` hardcode in `ReconnectEdgeTool.cpp:78` — we'll add the
   seam *and* paint the default through it ourselves so the bug class goes away.
2. **M15 — spatial hit-testing.** Agreed; `Graffodil::Spatial` already exists,
   the gap is that the interaction path doesn't consult it. No new structure,
   just rewiring hover/press to go through the index.
3. **M19 residual — parallel edges / self-loops in `edgeWaypoints`.** Agreed;
   layer assignment is a separate layer from the waypoint table and that table's
   keying is the remaining gap.

Performance rows (`countCrossings` BJM, `SelectMoveTool` guide-overlay rebuild,
`QuadTree::insert`/`computeRepulsion`) are accepted but deliberately sequenced
behind the three functional asks above, matching your own suggested order —
correctness before throughput.

`setEdgesAboveNodes()` z-ambiguity stays documented as an accepted limitation
(roadmap Phase 7, B6 second half) until a consumer actually hits it.

## On the repro offer

Yes, please attach failing scenarios against our public API wherever you can
(particularly M14/M15/M19-residual, where behaviour — not just line numbers —
is what we're reworking). It's cheaper for us to write the same test you'd have
written, and it makes the fix verifiable on our side before we ask you to
re-pin.
