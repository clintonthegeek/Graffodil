# Graffodil graph-editor follow-up handoff — 2026-07-01

We consumed the focused-editing and background-double-click work downstream.
The PlanEngine cleanup confirms those two surfaces are good enough to build on.

## What we changed downstream

- removed the PlanEngine-local `focusItem()`/`QGraphicsTextItem` keyboard bypass
  in `libs/graph/src/planstangraphscene.cpp`,
- kept the existing canvas double-click signal path working; no consumer breakage,
- ran the graph-scene test that covers the new routing.

## What we learned from using it

The remaining rough edges are now clearer:

1. **Edge-midpoint affordances still want a reusable API.**
   PlanEngine currently has plus/delete shims embedded in its edge item and
   scene/view wiring. The Graffodil side should own the reusable interaction
   surface, not the app.

2. **Node sizing still needs a contract, not just local layout math.**
   PlanEngine's node cards currently size themselves from content and wrap
   rules. A generic graph node API should expose preferred size / remeasure
   semantics so consumers don't duplicate that logic.

## Recommended next Graffodil pass

- add a small, reusable edge-action surface for midpoint insert/delete,
- add a node size-hint/content-bounds contract,
- keep the focused-editing and background-double-click contracts as-is.

## Notes

- PlanEngine will stay on the signal-based background-double-click flow for now.
  It is working and does not need another scene-layer rewrite.
- The next Graffodil changes should be designed from consumer pain, not from
  abstract API completeness.
