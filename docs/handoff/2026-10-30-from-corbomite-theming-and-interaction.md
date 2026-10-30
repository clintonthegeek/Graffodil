# From Corbomite — what we would like from Graffodil next

**From:** Corbomite (`/home/clinton/dev/Corbomite`)
**To:** Graffodil maintainers
**Date:** 2026-10-30
**Re:** the Graffodil asks in
`Corbomite/docs/handoff/2026-10-18-to-markoff-consolidated-open-asks.md`
(ASK-M5, M6, M14, M15, M19 plus a table of smaller items)

**What we run: Graffodil master tip (`57c8024`) as a Corbomite submodule at
`libs/graffodil`, and your 16 tests run inside our suite
(`193888b2 test(graffodil): run Graffodil's 16 tests in our suite`).**

## Received and verified — thank you

All four fixes from the first ask round are in our pin:

- `79357fc` — M5: `~BatchRenderer` no longer touches items the scene
  destroyed.
- `fc778db` — M6: edge endpoints are detached and tool targets resolve by id.
- `bb41c0d` — M19: Sugiyama layer assignment relaxes to a fixed point, so
  cycles no longer corrupt layering. Canvas files and dependency graphs
  legitimately contain cycles, and ours do.
- `f0095bf` — deterministic winner for overlapping hit-tests (one of the
  smaller rows, resolved before we could even file it formally).

Being able to consume each fix by re-pointing the submodule is the whole
reason we filed these instead of working around them.

## What we are asking for

### 1. M14 — a theming surface (the big one)

`FormatPass`-style hardcoding is invisible to us until it isn't. Today every
canvas colour in Graffodil is a literal, so our dark-mode work (`shipped
2026-10-02`) had to build its own 14-role `CanvasPalette`
(`Corbomite/libs/canvas/include/canvas/CanvasPalette.h`), and
`Corbomite/src/canvas/ReconnectEdgeTool.cpp:78` **re-hardcodes the identical
`QColor(0,120,215)`** because Graffodil's own connect-preview line is not
themeable. We are reimplementing your colour in our tool because there is no
seam.

**Ask:** a `Graffodil::Palette` struct plus `GraphScene::setPalette()`, and
`GraphEdgeItem::setSelectionColor()`, threaded to every colour site; for
`BatchRenderer`, an optional `std::function<QColor(const IGraphNode *)>`
colour provider so batch mode can honour per-node theming. **Our
`CanvasPalette` is the worked example** — take it, adapt it, or tell us what
shape you want and we will re-cut ours to match.

### 2. M15 — interaction that is not O(N) per event

Graffodil ships `Spatial` and then does not use it on the interaction path:
press-time hit-testing walks the item list, and canvas-scale graphs (hundreds
of nodes, cards with live text) pay for it on every mouse event. **Ask:** make
hover and press hit-testing delegate to the spatial index. We do not need a
new structure; we need the one you have to be on the hot path.

### 3. M19 residual — parallel edges and self-loops in `edgeWaypoints`

The fixed-point layer assignment landed; the waypoint table itself is still
keyed by `(sourceId, targetId)` (`SugiyamaLayout.cpp:790-798`, as verified
2026-10-18). Parallel A→B edges collapse onto one route and self-loops are
unhandled. Both occur in the graphs we render.

### 4. The smaller rows, listed with their evidence

These were verified against the pinned source on 2026-10-18; `f0095bf` has
since landed for the hit-test winner row, and other line numbers may have
drifted by a few lines — the asks stand, the cites are where they were
measured.

| Ask | Where | Why it matters to us |
|---|---|---|
| `edgeActionAt` hit-tests in **item-local** coords and copies every path per edge per press | `GraphScene.cpp:613-625` | `nodeSubItemAt` (`:634`) uses scene coords — the two disagree, so any rotated/translated edge has an unclickable affordance |
| `GraphMinimap` never resets `m_dragging` | `GraphMinimap.cpp:97` | No `mouseReleaseEvent` exists; after one click every hover re-centres the view with no button held |
| `GraphMinimap` divides by zero-width rects | `GraphMinimap.cpp:66-68,134-136` | `painter.scale(inf, inf)` on a degenerate scene rect |
| `GraphScene::clearGraph()` emits mid-teardown | `GraphScene.cpp:186-197` | `m_edges.clear()` runs *after* the emit loop — a re-entering slot corrupts the registries |
| `countCrossings` is O(E²), up to 25× per layout | `SugiyamaLayout.cpp:131-138,654,701` | Barth–Jünger–Mutzel makes it O(E log V) |
| `SelectMoveTool` rebuilds the guide overlay every drag frame | `SelectMoveTool.cpp:189-202` | removeItem + delete + new + re-add per mouse-move |
| `CreateEdgeTool` leaks its preview line | `CreateEdgeTool.cpp:21-31` | Cleans `m_highlight`, never `m_previewLine` |
| `GraphEdgeItem::boundingRect()` ignores pen width | `GraphEdgeItem.cpp:301-312` | Highlight/selection widen the pen **without** `prepareGeometryChange()`, so the stroke clips |
| `QuadTree::insert()` holds a `QuadNode&` across `subdivide()` | `QuadTree.cpp:76,96` | Safe only incidentally; the reserve is insufficient, so a realloc *will* happen |
| `QuadTree::computeRepulsion` uses `bounds.width()` as the region size | `QuadTree.cpp:161` | Non-square root bounds silently coarsen the Barnes-Hut approximation on wide graphs |
| `setEdgesAboveNodes()` cannot tell a consumer's z=0 from the Qt default | `GraphScene.cpp:101-112` | Qt's default z *is* 0.0, so the header's claim is unachievable as stated |
| `IGraphNode::anchorToward()` returns a silent `(0,0)` on an empty list | `IGraphNode.cpp:8-20` | Edges route to the scene origin; sibling `anchor()` (`:28-33`) *does* warn — but at per-frame rate |

A suggested order, if it helps you sequence: **M14 → the two correctness rows
(`edgeActionAt` coords, `clearGraph` emit) → the lifetime/perf rows
(`CreateEdgeTool` leak, `SelectMoveTool` rebuild, minimap pair,
`boundingRect` pen, `QuadTree` pair) → `countCrossings` last.** The first
group are things a user can hit; the last is throughput only.

## What we will do on our side

- Your suite already runs in ours, so every fix ships with a canary the day
  you publish it.
- Where a row is consumer-visible (canvas cards, edges out of the scene
  origin), we can provide a failing scenario against your public API rather
  than a patch — say the word and we will attach repro steps to the row in
  question.
- When M14 lands we will delete our hardcode in `ReconnectEdgeTool.cpp:78`
  and file the diff that shrinks `CanvasPalette` to only the roles you did
  not cover.

## How to answer

Edit this file, or drop a reply in `docs/handoff/` and we will find it. A
"this row is already fixed at master, re-point your pin" is the best answer
short of a fix, and we will re-verify the same day.
