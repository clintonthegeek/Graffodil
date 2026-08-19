# Reply: Graffodil → Corbomite — Canvas adoption (Cluster M) is unblocked

**Date:** 2026-08-19. **From:** Graffodil.
**In reply to:** `~/dev/Corbomite/docs/handoff/2026-08-19-to-graffodil-for-canvas-adoption.md`.
**Bottom line:** both decision items are resolved, nothing gates M0, and the
item-2 gap is already fixed upstream in `v0.2.3`.

## Decision item 1 — consumption mechanism: **submodule, blessed**

Take option (2). Do not wait on 6d.

- Graffodil already has a GitHub remote — `git@github.com:clintonthegeek/Graffodil.git`
  — and as of today it is at the same commit as Codeberg (`7012ee0` before this
  reply's changes). Use it as Corbomite's submodule URL, matching the
  `libs/markoff-family` precedent. Note the nuance: unlike Markoff, Graffodil's
  own `origin` **is** GitHub (Codeberg is the secondary remote), so there is no
  mirror-lag risk.
- Both hosts are public and clone anonymously over HTTPS — verified today with
  `git ls-remote` against `https://github.com/clintonthegeek/Graffodil.git` and
  `https://codeberg.org/clintonthegeek/Graffodil.git`. Your `.deb`-on-`v*`-tag CI
  can pull submodules without credentials from either.
- `add_subdirectory` consumption is already clean: `GRAFFODIL_BUILD_DEMO` and
  `GRAFFODIL_BUILD_TESTS` both default to `${PROJECT_IS_TOP_LEVEL}`, so a
  submodule build compiles the six libraries and nothing else. Link
  `Graffodil::Core` (the in-tree ALIAS names match the future exported names, so
  a later switch to `find_package` is a build-system change on your side only —
  no source change).

6d (install/export) stays on our roadmap for packagers and for consumers who
want a system-installed Graffodil, but it is no longer sequenced ahead of your
migration. We've reworded the roadmap accordingly.

**Version to pin:** `v0.2.3`, tagged today, containing the item-2 fix below.
Pin by SHA as you prefer; the tag is there for the changelog trail.

## Decision item 2 — edge-creation gesture: **both gestures already work; drop-on-empty now has a hook**

Two separate answers:

1. **Press-drag-release already completes an edge.** The class docs the handoff
   quoted are out of date relative to the code. `CreateEdgeTool` supports
   click-click *and* press-drag-release; a sub-threshold drag (< 4 scene units)
   falls through into click-click so users can mix the two. If you want
   Obsidian's strict behaviour — a release without a real drag cancels instead
   of arming click-click — call `setDragOnly(true)`. That's exactly what it's
   for. `setAnchorHoverRadius()` widens the hover-connection-point hit area.

2. **Release-on-empty was a silent cancel — now it's an intent signal.** Added
   upstream rather than pushed to a subclass:

   ```cpp
   void edgeDroppedOnEmpty(IGraphNode *source, const QString &sourceAnchorId,
                           const QPointF &scenePos);
   ```

   Fires when an in-progress gesture ends over empty canvas: drag released with
   no node under the cursor, **or** (click-click mode) the second click lands on
   the background. The tool resets itself *before* emitting, so your handler is
   free to run a modal create-menu at `scenePos`, call `placeNodeAt`, and build
   the edge — the tool never touches your model, consistent with
   `edgeRequested`. Leave it unconnected and drop-on-empty cancels exactly as
   before, so this is additive for every other consumer.

   Deliberate non-case: a drag that starts and ends on the *same* node is a
   rejected self-loop, not a drop on empty; it cancels silently and emits
   nothing.

   Covered by four new `tst_tools` cases (drag-drop, click-drop, idle click on
   background stays silent, self-loop is not a drop). Full suite 16/16.

   One wart worth knowing: `QSignalSpy` can't marshal `IGraphNode*` through
   `QVariant` without `qRegisterMetaType<IGraphNode*>("IGraphNode*")`. Direct
   connections are unaffected — it only bites tests that spy on the signal.

## Nice-to-haves

- **`IAlignmentStrategy` already exists** — `SelectMoveTool::setAlignmentStrategy()`
  filters the live drag position and supplies guide lines; Core deliberately
  ships no default implementation. So implement Obsidian's
  snap-to-grid/snap-to-objects as a consumer strategy exactly as planned; no
  upstream shaping needed. If it lands generic enough (grid pitch + corner/center
  candidate sets, no `.canvas` specifics), tell us and we'll consider adopting it
  as an optional Core-shipped default — but don't contort your implementation
  for that possibility.
- **Tag:** `v0.2.3` as above.

## What Graffodil is not doing

No eased/animated camera in `PanZoomTool` — driving `QVariantAnimation` over
`applyZoom(factor, anchorScenePos)` on your side remains the intended pattern.

Ping us if M1's rebase turns up further gaps; the deference stated in your
handoff runs both ways — API stability is not the constraint here.
