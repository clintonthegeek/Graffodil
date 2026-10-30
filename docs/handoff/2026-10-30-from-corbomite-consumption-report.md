# Consumption report from Corbomite — second round verified on our side

**From:** Corbomite
**To:** Graffodil maintainers
**Date:** 2026-10-30 (same-day follow-up)
**Re:** [`2026-10-30-reply-to-corbomite-theming-and-interaction.md`](2026-10-30-reply-to-corbomite-theming-and-interaction.md)

We re-verified rather than taking the summary's word for it. Verdict up
front: **the three functional asks are delivered and verified; the perf rows
are not "finished" and we do not treat them as such** — your reply and
`docs/ROADMAP.md` both say they are accepted and queued, and we read the code
rather than the summary before filing this.

## What we consumed

Corbomite re-pinned `libs/graffodil` from `57c8024` to **`a3208ab`**
(`ea5b7bae` in our repo) and themed our scene through the new seam
(`3273bad9`). Full numbers below.

## Verified by reading the source at `a3208ab`

**M14 — the theming surface, delivered as specified:**

- `Palette` (`src/core/include/graffodil/Palette.h`) with defaults that
  reproduce the pre-palette pixels — checked the six role groups (interaction
  highlights, minimap chrome, batch mode).
- `GraphScene::setPalette()`/`palette()`/`paletteChanged()` at
  `GraphScene.cpp:98-105`, selection pushed to registered/new
  `GraphEdgeItem`s, tools re-theme at gesture start, minimap paints from the
  scene palette.
- `BatchRenderer::setNodeColorProvider()` is not a stored stub: `synchronize()`
  calls it per node at `BatchRenderer.cpp:156`
  (`nd.color = d->nodeColorProvider ? d->nodeColorProvider(node) : pal.batchNode`).

**M15 — hit-testing off the hot path:**

- `nodeForGraphicsItem`/`edgeForGraphicsItem` resolve via maintained
  `m_nodesByItem`/`m_edgesByItem` registries — O(items under the cursor), not
  O(registry) per event — and the tools are actually rewired
  (`SelectMoveTool::findNodeAt`/`findEdgeAt` now call the accessors). One
  wording note: this is the registry-map seam, not the `Spatial` index; we
  consider the ask met ("no new structure, get the O(N) per-event scan off the
  path"), just flagged so nobody calls it what it is not.

**Batch 1 — all eight rows, checked one by one:**

| Row | What we found at the source |
|---|---|
| `edgeActionAt` item-local coords | maps to scene coords before the distance test (`GraphScene.cpp:692-700`) |
| minimap `m_dragging` never reset | `mouseReleaseEvent` added (`GraphMinimap.cpp:122-127`) |
| minimap degenerate rects | `<= 0` guards at both paint and drag paths (`:71`, `:152`) |
| `clearGraph()` emits mid-teardown | registries (`m_edges`, `m_edgesByItem`, `m_edgesByNode`, `m_nodes*`) cleared **before** the emit loops, with `detachEndpoints()` per edge (`GraphScene.cpp:200-232`) |
| `boundingRect()` ignores pen width | margin = `max(hitWidth/2, stroke/2 + 1)` including the widened selection stroke (`GraphEdgeItem.cpp:340-352`), with `prepareGeometryChange()` in the setters |
| `anchorToward()` silent `(0,0)` | warns once with the node id (`IGraphNode.cpp:8-16`) |
| `CreateEdgeTool` preview leak | destructor reclaims it (`CreateEdgeTool.cpp:34-38`) |
| non-deterministic hit-test winner | fixed in `f0095bf` before we even filed it — your "one day after the measurements" remark stands |

**M19 residual — per-edge routes:**

- `LayoutResult::edgeWaypointsByEdgeId` exists and `GraphScene` consumes the
  per-edge map first (`GraphScene.cpp:441-455`); self-loops get explicit
  routes and parallel-edge instances keep separate accumulators.

## What is NOT finished, and we are fine with that

- `countCrossings` is still O(E²) (`SugiyamaLayout.cpp:110`, `:704`, `:751`) —
  BJM queued.
- `SelectMoveTool` still rebuilds the guide overlay with delete+new per drag
  frame (`:186-200`).
- The two `QuadTree` rows (`insert()`'s reference across `subdivide()`,
  `computeRepulsion()`'s `bounds.width()` region size) are untouched.
- `setEdgesAboveNodes()` z-ambiguity: accepted limitation, documented — the
  right call; nobody has hit it yet.

Your ROADMAP records all four as queued, which matches what the code says.
Our suggested order stands; correctness rows first was your choice too.

## Our side is done and green

- Graffodil's own suite: **16/16** at `a3208ab`.
- Corbomite's full offscreen suite on the new pin with the palette wiring:
  **281/281** (`ctest -E "benchmark|perf"`), wrt the only excluded items being
  the tagged benchmark/perf targets.
- Our hardcode — the `QColor(0,120,215)` reconnect preview at
  `libs/canvas/src/ReconnectEdgeTool.cpp` — is **deleted**; the preview now
  paints `palette().connectPreview`. `CanvasScene::setCanvasPalette` derives a
  `Graffodil::Palette` from our `CanvasPalette`: `borderSelected` feeds
  `edgeSelection`/`anchorDot`/`connectPreview`/`rubberBand`, `background` feeds
  `anchorDotFill`, and the roles we do not carry keep your defaults. That is
  the "diff that shrinks the role set to what a consumer actually owns" — we
  deliberately did not mint boilerplate roles to mirror yours.

Nothing new to hand you in this round beyond the standing offer: your tests
run in our suite, and any regression of the six verified rows turns red here
inside a day.
