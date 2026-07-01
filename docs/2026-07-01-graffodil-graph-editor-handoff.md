# Graffodil Graph Editor Handoff — 2026-07-01

This is the handoff for the Graffodil-side graph editor refit. The ask is
deliberately API-changing: improve the library even if consumers break
temporarily.

## End goal

Make the graph stack handle three things cleanly and generically:

1. in-card text editing on node double-click,
2. blank-canvas double-click to create a node,
3. edge-midpoint insertion affordances.

PlanEngine currently works, but it does so by layering app-specific
workarounds over Graffodil's current event model.

The recent local fixes (372f8a2, 4c5d7a2, 127e8c9, d84103c, 221eacb,
3c35093) are all variations on the same theme: make the in-card editor behave
acceptably inside the current pipeline. They are useful history, but they also
prove the pipeline itself needs to change.

## Why the current shape is brittle

- Graffodil routes scene key presses straight to the active tool
  (`GraphScene::keyPressEvent` in `src/core/src/GraphScene.cpp:476-481`), and
  the default delete behavior lives in `SelectMoveTool::keyPressEvent`
  (`src/core/src/SelectMoveTool.cpp:248-263`).
- PlanEngine has to bypass that path when a node title editor is focused:
  `PlanStanGraphScene::keyPressEvent` special-cases a focused
  `QGraphicsTextItem` and forwards typing directly to the scene item path
  instead of the tool layer (`libs/graph/src/planstangraphscene.cpp:1297-1310`).
- Graffodil only treats a double-click as a background click when
  `items(scenePos).isEmpty()` (`src/core/src/GraphScene.cpp:492-500`), so
  PlanEngine has to re-emit that as `canvasDoubleClicked` and handle the
  actual node creation itself (`libs/graph/src/planstangraphscene.cpp:98-103`,
  `libs/graph/src/dependencygraphview.cpp:636-657`, `788-835`).
- Edge-midpoint insertion is already represented as a special mid-edge
  glyph in PlanEngine (`libs/graph/src/blockedge.cpp:205-253`), but the
  actual command plumbing is app-local (`libs/graph/src/planstangraphscene.cpp:258-267`,
  `652-660`; `libs/graph/src/dependencygraphview.cpp:838-897`).
- Node sizing and wrapping are also app-owned: `BlockNode::computeCardSize()`
  computes a width/height from text length and `QTextDocument` wrapping
  (`libs/graph/src/blocknode.cpp:236-313`), and the paint path suppresses
  stored title rendering while the editor child is active
  (`libs/graph/src/blocknode.cpp:430-456`, `785-855`).

## Requested Graffodil changes

### 1) Make focused text editing a first-class scene concern

Goal: editable text should not require consumer-specific key-routing hacks.

Requested shape:

- a clean way for a focused `QGraphicsTextItem` (or equivalent editable item)
  to preempt tool-level Delete/Backspace handling,
- an explicit edit-start / edit-finish contract if Graffodil wants one,
- no requirement that consumers special-case `focusItem()` in their scene
  subclass just to keep typing working.

Relevant current code:

- `GraphScene::keyPressEvent` in `src/core/src/GraphScene.cpp:476-481`
- `SelectMoveTool::keyPressEvent` in `src/core/src/SelectMoveTool.cpp:248-263`
- PlanEngine workaround in `libs/graph/src/planstangraphscene.cpp:1297-1310`
- Node-local editor creation in `libs/graph/src/blocknode.cpp:785-855`

### 2) Make background double-click a real background gesture

Goal: empty-canvas double-click should be a real scene-level gesture, not an
`items(scenePos).isEmpty()` heuristic that each consumer has to reinterpret.

Requested shape:

- a background double-click signal / callback that is independent of the
current item-hit test implementation,
- or a more explicit gesture dispatch from the scene/tool layer.

Relevant current code:

- `GraphScene::mouseDoubleClickEvent` in `src/core/src/GraphScene.cpp:492-500`
- forwarding shim in `libs/graph/src/planstangraphscene.cpp:98-103`
- create-on-double-click flow in `libs/graph/src/dependencygraphview.cpp:636-657`,
  `788-835`

### 3) Make edge-midpoint actions first-class edge affordances

Goal: the `+` / delete glyphs should be supported by Graffodil as a generic
edge interaction pattern, not as a PlanEngine-local shim.

Requested shape:

- an edge-action API (insert-between, delete, maybe later others) that Graffodil
 can position and hit-test,
- or a generic mid-edge action hook so the consumer only supplies semantics.

Relevant current code:

- glyph placement in `libs/graph/src/blockedge.cpp:205-253`
- scene re-emit in `libs/graph/src/planstangraphscene.cpp:258-267`, `652-660`
- consumer command handling in `libs/graph/src/dependencygraphview.cpp:838-930`

### 4) Add a stable size-hint / content-bounds contract for nodes

Goal: consumers should be able to ask a node how much space it needs after
content changes, without hardcoding layout math around a specific node class.

Requested shape:

- a size-hint or preferred-rect contract on graph nodes,
- support for content-driven remeasure / relayout hooks,
- no assumption that the library owns the node's text semantics.

Relevant current code:

- `BlockNode::computeCardSize()` in `libs/graph/src/blocknode.cpp:236-313`
- wrapping/editor-width coupling in `libs/graph/src/blocknode.cpp:430-456`,
  `785-805`

## PlanEngine follow-up after Graffodil lands

1. Update the graph scene to use the new focused-editing contract and delete
   the current text-editor key-routing bypass in
   `libs/graph/src/planstangraphscene.cpp:1297-1310`.
2. Replace the current blank-canvas create flow with the new Graffodil
   background-double-click API; keep the block-creation semantics in
   `DependencyGraphView`, but remove the local scene-heuristic plumbing.
3. Move edge-midpoint affordance handling onto the new Graffodil edge-action
   surface, then delete the PlanEngine-specific glyph/signal shim chain.
4. Rework node sizing to consume the new size-hint contract and delete any
   duplicated wrap/measurement assumptions that remain.
5. Only then do the final graph-polish pass: layout tuning, gesture polish,
   and any remaining editor ergonomics.

## Notes for Graffodil maintainers

- Breaking API is acceptable here if it buys a cleaner, more reusable graph
  interaction model.
- The target is not just to make PlanEngine work; it is to make the library
  itself better for any consumer that needs editable nodes and edge actions.
- If a change feels like "PlanEngine special handling," that's a smell.
