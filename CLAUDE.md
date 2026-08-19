## Building

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

## Testing

```bash
cd build && ctest --output-on-failure
```

## Architecture

Core + Extensions model. See `docs/graffodil-design.md` for the full spec.

- `Graffodil::Core` — interfaces, scene, tools, edge rendering, utilities
- `Graffodil::Spatial` — QuadTree, SpatialGrid (depends: Qt6::Core only)
- `Graffodil::Sugiyama` — Sugiyama layered layout (depends: Core)
- `Graffodil::Force` — ForceAtlas2 + multilevel (depends: Core, Spatial)
- `Graffodil::Circular` — Circular layout (depends: Core)
- `Graffodil::Batch` — Batch rendering for 10K+ nodes (depends: Core)

## Current Status (2026-08-19)

The library is feature-complete against `docs/graffodil-design.md` and got a
full evaluation on 2026-06-09 (`docs/evaluation-2026-06-09.md`: bugs,
architecture findings, performance items, feature avenues).

**Task status lives in [`docs/ROADMAP.md`](docs/ROADMAP.md) — the single
source of truth.** Start there; it sequences the phases, links every plan and
spec, and carries the status log. Phases 1–5 plus 6c/6f are complete; the open
phases are 6a, 6b, 6d, 6e (all spec'd, each needs a plan).

Downstream consumers migrate now, not later: as of 2026-08-19 Corbomite's
canvas adoption is underway and consumes Graffodil as a **git submodule +
`add_subdirectory`** (6d/install is for packagers, not a gate). Consumer asks
arrive as handoff docs in `docs/handoff/` — deference to a live consumer's
needs outranks API stability at this stage.

## Document Map

| Doc | Purpose |
|-----|---------|
| `docs/ROADMAP.md` | **Task status + sequencing (keep updated!)** |
| `docs/evaluation-2026-06-09.md` | Findings of record (IDs B*/A*/P* used by the roadmap) |
| `docs/graffodil-design.md` | Architecture/API spec |
| `docs/specs/*.md` | Feature specs awaiting implementation plans |
| `docs/superpowers/plans/*.md` | Executable implementation plans (checkbox tracking) |
| `docs/bugs/*.md` | Bug reports with specified fixes |
| `docs/corbomite-integration-feedback.md`, `docs/research/*.md` | Consumer requirements |

## Working Conventions

- When you complete a plan task, tick its checkbox in the plan file; when a
  phase finishes, update its row + the status log in `docs/ROADMAP.md`.
- New feature ideas: write a spec in `docs/specs/` first, then a dated plan in
  `docs/superpowers/plans/` (superpowers:writing-plans), then link both from
  the roadmap.
- Bugs get a report in `docs/bugs/` with a specified fix; append a Resolution
  section when fixed.
- Keep design-doc sections in sync when public API changes (each plan's
  close-out task lists what to touch).
