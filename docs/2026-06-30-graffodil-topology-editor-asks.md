# Feature Requests for Graffodil — Dependency-Graph Topology Editing
**Date:** 2026-06-30
**To:** Graffodil maintainers
**From:** PlanEngine (downstream consumer)
**Status:** Proposal — for discussion / possible upstream implementation

---

## Context & motivation

PlanEngine's dependency graph (`libs/graph`) is built on Graffodil's `GraphScene`, `SugiyamaLayout`, `GraphNode`, `GraphEdge`, and related infrastructure. The current surface is an excellent *viewer* (orientation-agnostic Sugiyama, group regions for compound milestones, proximity/ego lenses, adherence overlay). It is a weak *topology editor*.

Today, the only creation path is:
- `CreateSuccessorCommand` / `CreatePredecessorCommand` (Enter / Shift+Enter on a selected node)
- Rootless-`?` satellite nodes for unattached blocks

There is no canvas double-click creation, no edge mid-point affordance, no hover-delete on edges, and cards are fixed-size with elision. Zoom/pan behavior is fragile when the scene rect is not expanded.

The desired model (consistent with the existing `ViewPanelBase` / `IViewServices` seam) is:
- **Heavyweight editing stays in `QWidget` forms** (`BlockSqlEditor`, `BlockFormWidget`).
- **The graph becomes the canonical fast surface for create / delete / relate / unrelate** — the spatial, gestural complement to the form view.

We are prepared to implement the PlanEngine-side wiring (new commands, signal routing, `BlockStoreSql` integration) ourselves. The Graffodil-side gaps that would otherwise force us to fight the framework are listed below.

All references are to the current PlanEngine `master` (commit range after the R4 desktop-shell work, June 2026) and the Graffodil checkout that PlanEngine builds against.

---

## Ask G1 — Canvas double-click creation signal + documented placement hook

**Current state.** `GraphScene` (and therefore `PlanStanGraphScene`) has no public signal for background double-click. `placeNewNode` exists but is private and neighbor-anchored only.

**Desired Graffodil change.** Expose (or document) a way for a subclass to receive a `backgroundDoubleClicked(QPointF scenePos)` or equivalent, and provide a public or protected `placeNodeAt(QPointF scenePos, ...)` variant that accepts an explicit coordinate instead of computing from a neighbor.

**PlanEngine-side work (we own this).**
- `planstangraphscene.cpp`: implement the handler, emit `canvasDoubleClicked(QPointF)`.
- `dependencygraphview.cpp`: forward the signal; wire to `CreateUnattachedBlockCommand`.
- `blocknode.cpp`: ensure `beginTitleEdit()` is safe on a freshly-constructed node.

**Why this belongs in Graffodil.** A consumer that wants "double-click to create" should not have to install a raw `QGraphicsScene` event filter and duplicate coordinate-mapping logic. The placement helper is already there; making the coordinate explicit is a small, non-breaking addition.

---

## Ask G2 — Edge hover affordances (mid-point "+" insert-between + "×" delete)

**Current state.** `GraphEdge` has no hover enter/leave, no parametric point query, and no transient child-item protocol. `BlockEdge` therefore cannot easily own a mid-point widget.

**Desired Graffodil change.** Provide (or document) a lightweight way for an edge subclass to:
1. Receive hover enter/leave events.
2. Obtain a `QPointF edgePoint(qreal t)` along the rendered path (0.0 = source, 1.0 = target).
3. Own transient `QGraphicsItem` children that are automatically hidden when the edge loses hover (or provide a helper to create a "floating widget on edge").

**PlanEngine-side work (we own this).**
- `blockedge.h/cpp`: implement the hover logic, compute the midpoint, create the `+` / `×` overlay items, emit `insertRequested(fromId, toId)` / `deleteRequested(fromId, toId)`.
- `planstangraphscene.cpp`: new `insertNodeOnEdge(const QString &fromId, const QString &toId)` helper.
- `dependencygraphview.cpp`: route the signals to `InsertBetweenCommand` / relation removal.

**Why this belongs in Graffodil.** Every consumer that wants "click the middle of an edge to insert" ends up writing the same hover + parametric-point + transient-item boilerplate. A small documented protocol (or a pair of protected virtuals) would eliminate the duplication.

---

## Ask G3 — Dynamic node sizing (remove fixed CardWidth/CardHeight)

**Current state.** `BlockNode` hard-codes `static constexpr CardWidth = 175; CardHeight = 52;` and uses them in `boundingRect`, anchors, satellite hit rects, and text elision. Graffodil's `SugiyamaLayout` already accepts per-node sizes via `setNodeSize`, but the PlanEngine node never reports a variable size.

**Desired Graffodil change.** None strictly required — the layout engine already supports variable sizes. The ask is only that any future Graffodil change that assumes fixed node sizes documents the assumption so downstream dynamic-size nodes remain supported.

**PlanEngine-side work (we own this).**
- `blocknode.h/cpp`: replace the constants with a computed `computeCardSize()` based on `QFontMetrics` + `QTextOption::WrapAtWordBoundaryOrAnywhere`; update `boundingRect`, `anchors`, `paint`, and the satellite rectangle.
- `planstangraphscene.cpp`: pass the actual size to `SugiyamaLayout::setNodeSize` (already done per-node); the collision walk in `placeNewNode` uses the real height.

**Why this is low-risk for Graffodil.** The change is entirely in the PlanEngine `BlockNode` subclass. We simply want reassurance that Graffodil will continue to treat node sizes as data rather than baking a fixed-size assumption into new layout or rendering code.

---

## Ask G4 — Scene-rect expansion on zoom / pan (robust viewport growth)

**Current state.** `DependencyGraphView` uses `AnchorUnderMouse` and a fixed scene rect. Repeated zoom toward the top-left corner eventually pins content against the scene boundary; panning after deep zoom can push nodes outside the visible area with no automatic expansion.

**Desired Graffodil change.** None — this is a `QGraphicsView` / `DependencyGraphView` responsibility. The ask is only that Graffodil's examples or documentation note the common pattern of expanding `sceneRect()` on transform changes so consumers do not have to discover it by painful trial and error.

**PlanEngine-side work (we own this).**
- `dependencygraphview.cpp`: after every `zoomIn/Out`, wheel zoom, or pan that would move content outside the current scene rect, compute the new viewport rect in scene coordinates, union it with `sceneRect()`, and add a generous margin (500–1000 px). Do the same in `fitAll`.

**Why this note is useful.** Several Graffodil examples and the default `GraphView` behavior leave the scene rect static. A one-sentence "remember to grow the scene rect on zoom" in the docs would have saved the author of `DependencyGraphView` a full day of head-scratching.

---

## Non-asks (explicitly out of scope for this proposal)

- **Adherence overlay glyph** (`INodeAdherenceSource`, dashed amber ring + pulse dot) — already implemented in PlanEngine's `BlockNode::paint` using Graffodil's existing `GraphNode` paint hooks; no upstream change needed.
- **Compound milestone group regions** — already landed in Graffodil (`GraphScene` group regions) and wired in PlanEngine; no further work.
- **Orientation flip** — already supported via `SugiyamaLayout::setDirection` (or equivalent) and exposed in the PlanEngine toolbar; no further work.
- **Any change to Graffodil's core data model or layout algorithm** — we are happy with the current Sugiyama + region + lens surface.

---

## Summary table

| Ask | Graffodil change | PlanEngine work | Risk to Graffodil |
|-----|------------------|-----------------|-------------------|
| G1 — canvas double-click | expose/document `backgroundDoubleClicked` + explicit `placeNodeAt` | high (commands, title-edit) | low (small API addition) |
| G2 — edge hover `+` / `×` | hover events + parametric point + transient-item protocol | high (overlay items, insert command) | medium (new protected surface) |
| G3 — dynamic card size | none (just reassurance) | medium (font metrics, boundingRect) | none |
| G4 — scene-rect growth | none (documentation note) | low (viewport math) | none |

---

## Closing

We are excited about Graffodil's trajectory and the clean separation it gives us between layout/rendering and domain semantics. The four small affordances above would let the dependency graph become the primary fast topology surface in PlanEngine without forcing us to fight the framework. We are happy to prototype any of the Graffodil-side changes on a branch and bring them back for review, or to implement the PlanEngine side first and demonstrate the desired behavior with a thin shim.

Please let us know how you would like to proceed.

— Clinton (PlanEngine)
