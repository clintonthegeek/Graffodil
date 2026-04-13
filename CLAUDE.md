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
- `Graffodil::Sugiyama` — Sugiyama layered layout (depends: Core)
- `Graffodil::Force` — ForceAtlas2 + multilevel (depends: Core, Batch)
- `Graffodil::Circular` — Circular layout (depends: Core)
- `Graffodil::Batch` — Batch rendering for 10K+ nodes (depends: Core)
