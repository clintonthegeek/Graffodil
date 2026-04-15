# Corbomite integration feedback

**Written from:** Corbomite (`~/dev/Corbomite/`) — prospective Graffodil consumer.
**Date:** 2026-04-14.
**Source of recommendations:** planning docs `~/dev/Corbomite/docs/superpowers/plans/2026-04-14-cluster-p-graffodil-adoption-SCOUTING.md` and the underlying audits at `~/dev/Corbomite/docs/obsidian-audit/domains/*.md` (for understanding what Corbomite's graph and canvas views must support).
**Corbomite status:** v0.x, pre-adoption. `libs/forcegraph/` (~3,283 LOC) and `libs/canvas/` (~3,851 LOC) are the planned consumers per Graffodil's own `docs/graffodil-design.md` §Boundary Map.

---

## Purpose

As a prospective consumer preparing to migrate two graph-rendering libraries onto Graffodil, Corbomite has surveyed the places where its use cases push or stretch Graffodil's current API. Some of these are genuinely general needs other consumers will hit (upstream-worthy); others are Corbomite-specific whiteboard/document-canvas features that would pollute Graffodil's generality if absorbed. This document separates them honestly and recommends where each should live.

**North star (respected from `graffodil-design.md` §Non-Goals):**
- Graffodil does not dictate what nodes look like — consumers own node rendering.
- Core stays lightweight; optional modules for heavy features.
- Don't force consumers to carry layout or batch rendering code they don't need.

Anything that would violate one of these non-goals lives in Corbomite, not Graffodil.

---

## Recommendations for Graffodil (upstream-worthy)

Each of these is a general need — at least two of Graffodil's five named consumers (PERT, Gantt, sync topology, ForceGraph, Canvas) would benefit.

### 1. Cached-content-rendering hook for rich-content nodes

**Corbomite driver:** `libs/canvas/src/TextCardItem.cpp` (302 LOC) renders markdown into a `QTextDocument` and caches it. `libs/canvas/src/FileCardItem.cpp` caches a thumbnail from the referenced vault file. `libs/forcegraph/src/BatchNodeItem.cpp` caches a pre-rendered dot + label pixmap. All three pay for cache-from-rendering through ad-hoc code that would normally belong behind an interface.

**General need:** Any consumer with nodes that render more than a simple ellipse/rectangle needs a cache. PERT task nodes (PlanStan) render task name + duration + status glyph — also cache-friendly. A future sync-topology widget would cache device icons. Only the most trivial node types (pure geometry) don't benefit.

**Suggested shape:** A mixin / capability interface, not a subclass requirement.

```cpp
namespace Graffodil {

// Optional mixin for IGraphNode implementers who render expensive content.
// The Batch module queries for this interface via dynamic_cast; if a node
// provides it, BatchRenderer composits the cached pixmap instead of calling
// paint() per frame. Nodes that don't implement it fall through to their
// normal paint path (unchanged behaviour).
class IRenderableCache {
public:
    virtual ~IRenderableCache() = default;

    // Return a pixmap representation at the requested logical size.
    // Implementations should cache and invalidate only on content change.
    virtual QPixmap cachedPixmap(const QSize &requestedSize) = 0;

    // Logical size hint (distinct from boundingRect if the node renders
    // with padding/scaling).
    virtual QSize contentSize() const = 0;

    // Cache-invalidation hook: consumer calls this when content changes.
    virtual void invalidateCache() = 0;
};

}
```

**Integration cost:** ~1 day. `BatchRenderer::synchronize()` checks `dynamic_cast<IRenderableCache*>(node)` per node; falls back to existing path when null. Backwards-compatible; zero disruption for current consumers.

**Rationale for upstream:** PERT, Force, and Canvas all have this need; better one interface than three bespoke caches. Keeps with §Non-Goal "don't dictate what nodes look like" — nodes still paint themselves; this is only a cache-query contract.

---

### 2. Explicit undo-integration signal contract (documentation + completeness audit)

**Corbomite driver:** `libs/canvas/src/CanvasCommands.cpp` owns Corbomite's undo stack. It currently wires per-mutation via direct CanvasScene hooks. Migration to Graffodil means every mutation path in a Graffodil tool must emit a signal Corbomite can use to construct an undo command.

**General need:** Universal. Every Graffodil consumer needs a full undo-redo integration. PlanStan uses QUndoStack; Corbomite uses QUndoStack; a sync-topology widget would want one.

**Suggested shape:** A documentation deliverable + an audit. Possibly one or two additional signals. Specifically:

1. Explicitly document in `graffodil-design.md` **every mutation signal** each provided tool emits. Current §Tool Framework mentions `nodeMoved`, `edgeRequested`, `deleteRequested` — are those exhaustive? What about:
   - Edge redirect (dragging an edge's endpoint to a different node)?
   - Anchor change (dragging an edge's endpoint to a different anchor on the *same* node)?
   - Rubber-band multi-select → multi-move in one drag (one signal per node, or one aggregate?)?
   - Tool activation itself (for undo to capture tool-change as an undo unit if needed)?

2. Consider providing signal granularity both ways:
   - **Per-atomic-mutation** signal (`nodeMoved` as it fires today — useful for real-time coaching, alignment hints).
   - **Per-user-action boundary** signals (`dragBegan(node)` / `dragEnded(node)`) so consumers can push *one* undo command per user drag, not per pixel.

**Integration cost:** ~2 hours doc + 2–4 hours audit-and-patch if gaps exist.

**Rationale for upstream:** Every consumer needs clear signal-for-undo mapping. The §Tool Framework section claims tools "never mutate the data model directly" — a complete signal catalog makes that promise verifiable.

---

### 3. Alignment snap-guide extensibility

**Corbomite driver:** `libs/canvas/src/CanvasScene.cpp` has alignment-guide overlay lines (visual rulers that appear during multi-select drag when dragged items align with other scene items). Users rely on the visual feedback for pixel-perfect card placement.

**General need:** PERT graphs don't need it (layered layout does the placing). Force graphs don't need it (engine places nodes). But Canvas-shaped consumers (Corbomite's Canvas, any future whiteboard/diagram tool) do.

**Status check:** `graffodil-design.md` §Provided Tools says `SelectMoveTool` supports "optional alignment snap guides". **Verify this matches Canvas's UX before writing the recommendation.** Specifically:

- Which scene items does it consider as alignment targets? (All nodes, selected nodes, a user-supplied callback?)
- At what snap distance? (Hardcoded, or settable?)
- What visual treatment? (Ruler line, dashed segment, label?)
- Can the consumer customize or disable the guide rendering?

If Graffodil's current implementation is fully customizable, **no change needed** — documentation is enough. If it's hardcoded, a small API extension to let the consumer supply alignment-target queries and a custom guide-paint callback would cover it.

**Integration cost:** unknown until verified. 0 hours (docs) to ~1 day (add extension points).

**Rationale for upstream conditional on verification:** If Graffodil's tool already has the hooks, skip. If not, the extension is small and genuinely benefits Canvas-shaped consumers.

---

### 4. (Aspirational, long-term) Qt Quick rendering fallback for ≥100k-node scenes

**Corbomite driver:** Obsidian vaults commonly hit 10k–50k notes for dedicated users. Corbomite's force graph must stay responsive at that scale. Graffodil's Batch module delivers this today for QGraphicsView. But at ≥100k nodes (plugin-generated "include every referenced URL as a node" graphs), QGraphicsView plateaus regardless of batching.

**General need:** Any high-scale consumer hits this eventually.

**Suggested shape:** Not a near-term recommendation. A longer-term `Graffodil::Quick` module that offers a parallel `QQuickItem`-backed rendering path for extreme-scale cases, with the same `GraphScene`/`IGraphNode` interface sitting in front. Scene-graph-level composition, GPU-accelerated.

**Integration cost:** several engineer-months. Not proposing now; flagging as a future epic so the Core-API shape stays compatible with a Quick-backed alternative.

**Rationale for upstream:** If we ever need it, it belongs in Graffodil — otherwise every consumer reinvents it. Keeping this on the radar now shapes API choices (e.g. avoiding QGraphicsItem-specific types in `IGraphNode`'s public contract — Graffodil already does this correctly via the `graphicsItem()` bridge method).

---

## Kept internal to Corbomite (would overload Graffodil if upstreamed)

Each of these is a **Canvas-shaped feature** — a document/whiteboard interaction idiom that does not transfer to graph-like consumers (PERT, force, circular topology, future sync). Upstreaming them would either clutter Graffodil's core or force its generalisation into "a generic diagramming library" rather than "a graph scene framework". Both undercut Graffodil's stated non-goals.

### 1. Group / container nodes (`GroupItem`)

**Corbomite driver:** `libs/canvas/src/GroupItem.cpp`. A group is a rectangle containing child cards; dragging the group moves all children; resizing the group scales with children. Used for whiteboard-style thematic grouping.

**Why not upstream:**
- PERT graphs have no grouping (tasks are atomic).
- Force-directed graphs have no grouping (nodes are atomic points).
- Sync topology has no grouping.
- Only whiteboard/diagramming consumers have groups — an outlier shape.

Upstreaming would add a whole `IGraphGroup` abstraction, child-propagation semantics, and resize-with-children logic into `Graffodil::Core` for the benefit of one consumer class. The principle "core stays lightweight" argues against it.

**Corbomite-side approach:** `GroupItem` remains in `libs/canvas/` as a Corbomite-specific `QGraphicsItem` subclass. It implements `Graffodil::IGraphNode` to participate in the scene (it's "a node" from Graffodil's perspective — a simple rectangle with an id). Child-group behaviour is handled by Corbomite's `CanvasScene` subclass intercepting `SelectMoveTool::nodeMoved` for the group and propagating to children itself. Graffodil doesn't need to know groups exist.

**If group semantics ever become useful to another consumer**, propose an optional `Graffodil::Grouping` module analogous to `Batch` / `Sugiyama` — separate, not in Core.

---

### 2. Resizable-node decorator / 8-point resize handles

**Corbomite driver:** `libs/canvas/src/TextCardItem.cpp` + `FileCardItem.cpp` both implement 8-point resize handles (corner + edge handles, Kdenlive-style). Canvas cards are sized by the user; other items in the scene are not.

**Why not upstream:**
- Force-directed graph nodes are circular and uniform-sized — no resize.
- PERT task nodes are sized by their content — user can't resize, only renaming changes size.
- Sync topology nodes are fixed-size device icons — no resize.
- Only whiteboard consumers resize nodes.

Upstreaming a `ResizableNodeDecorator` utility would produce 100–200 LOC of Graffodil code used by one consumer. Worse, giving it "generic" framing would pressure Graffodil into a broader diagramming-toolkit identity.

**Corbomite-side approach:** Resize handles are implemented directly in `TextCardItem` / `FileCardItem` (as they are today). Each card paints its own handles, hit-tests them, and emits resize events Corbomite's undo system captures. Graffodil is not aware.

---

### 3. Obstacle-aware edge routing

**Corbomite driver:** Canvas's bezier edges currently can overlap cards — visually ugly but tolerable. Obstacle-aware routing (bending edges around cards) would be a nicer Canvas experience.

**Why not upstream yet:**
- PERT has orthogonal routing that avoids overlap by construction (Sugiyama's channel-based routing is already obstacle-aware within its layered model).
- Force graphs don't route around nodes (edges are straight and cross freely — by design, it's a force-graph visual idiom).
- Circular/sync topology doesn't route.
- Only Canvas-shaped consumers need it.

**Corbomite-side approach:** Graffodil already has the correct extension point — `EdgePathStrategy`. Corbomite can implement `ObstacleAwareBezierStrategy : public EdgePathStrategy` entirely in `libs/canvas/`, overriding `computePath()` with an obstacle query against the scene. **No upstream change needed.**

**Reconsider for upstream** if (a) the Corbomite implementation proves clean and general (doesn't leak Canvas-specific assumptions), AND (b) another consumer asks for it. Both conditions, not either.

---

## Summary table

| Recommendation | Driver (Corbomite file) | Upstream? | Reason |
|---|---|---|---|
| `IRenderableCache` hook | `TextCardItem`, `FileCardItem`, `BatchNodeItem` | **Yes — upstream** | General need (any rich-content node); tiny interface; backwards-compatible |
| Undo-signal contract + audit | `CanvasCommands` | **Yes — upstream** | Universal need; mostly documentation |
| Alignment snap-guide extensibility | `CanvasScene` | **Verify first, likely upstream** | If Graffodil's current impl is rigid, small extension; if flexible already, docs |
| Qt Quick rendering fallback | `libs/forcegraph/` at scale | **Long-term; API-shape awareness now** | Multi-month effort; flagged for future, not near-term |
| `GroupItem` / container nodes | `GroupItem` | No — Corbomite-internal | Whiteboard-specific; implementing upstream pollutes Core generality |
| Resizable-node decorator | `TextCardItem`, `FileCardItem` | No — Corbomite-internal | Whiteboard-specific; one-consumer feature |
| Obstacle-aware bezier routing | Canvas edges | No (initially) — implement as local `EdgePathStrategy` subclass | Graffodil's extension point already covers this. Reconsider only if clean + requested by another consumer |

---

## How we'll collaborate during migration

Corbomite's Cluster P scouting doc (`~/dev/Corbomite/docs/superpowers/plans/2026-04-14-cluster-p-graffodil-adoption-SCOUTING.md`) prescribes the migration order per `graffodil-design.md` §Migration Path:

1. **Canvas first** (lighter consumer, de-risks framework on Corbomite side).
2. **ForceGraph second** (validates Batch at 10k+ nodes).

Expansion from scouting to a full migration plan is gated on:
- Graffodil API stable for 2–3 weeks (recent `Side`→`Anchor` refactor on 2026-04-13 is exactly the kind of upstream change that would hurt mid-migration).
- Cluster A (link-resolver / frontmatter parsing) landed in Corbomite — `GraphDataBuilder` depends on it.

When the expansion does happen, Corbomite will propose upstream patches for the three "Yes — upstream" items above, and handle the three "No — internal" items entirely within Corbomite's tree. Any friction we hit mid-migration that generalises goes into a follow-up addition to this doc.

**Contact protocol:** PRs against Graffodil for the three upstream items, with this doc referenced as the design context. Feedback on the three internal-only decisions welcomed — if you ever want any of those three in Graffodil, we'll reconsider.
