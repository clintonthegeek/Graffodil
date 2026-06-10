# Spatial Module Extraction Implementation Plan (Phase 5)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Break the Force→Batch dependency (evaluation finding A3): `QuadTree` and `SpatialGrid` move out of the Batch *rendering* module into a new `Graffodil::Spatial` utility module that both Force and (future) routing code consume.

**Architecture:** Pure build-system + file move — zero code changes inside the moved files (headers keep the `graffodil/` include prefix and the `Graffodil` namespace, so no `#include` or symbol changes anywhere). New static lib target `graffodil-spatial` linking only `Qt6::Core`. Force links `Graffodil::Spatial` (PRIVATE) instead of `Graffodil::Batch`; Batch stops compiling QuadTree.

**Tech Stack:** CMake 3.19+, Qt 6.8. Build dir `build/`.
**Sequencing:** Any time; roadmap puts it after Phase 4. Must precede the obstacle-routing feature (spec 6e).

---

### Task 1: Create the Spatial module and move the files

**Files:**
- Create: `src/spatial/CMakeLists.txt`
- Move: `src/batch/include/graffodil/QuadTree.h` → `src/spatial/include/graffodil/QuadTree.h`
- Move: `src/batch/src/QuadTree.cpp` → `src/spatial/src/QuadTree.cpp`
- Move: `src/batch/include/graffodil/SpatialGrid.h` → `src/spatial/include/graffodil/SpatialGrid.h`
- Modify: `CMakeLists.txt` (root), `src/batch/CMakeLists.txt`, `src/force/CMakeLists.txt`, `tests/CMakeLists.txt`

- [x] **Step 1: Verify the moved code only needs QtCore**

```bash
grep -h '#include' src/batch/include/graffodil/QuadTree.h \
                   src/batch/src/QuadTree.cpp \
                   src/batch/include/graffodil/SpatialGrid.h | sort -u
```

Expected: only QtCore types (`<QPointF>`, `<QRectF>`, `<QVector>`,
`<QHash>`, standard headers) and each other. If anything from QtWidgets/QtGui
appears, STOP and re-plan the link line (add the needed Qt component to
Step 3's `target_link_libraries`).

- [x] **Step 2: Move the files with git**

```bash
mkdir -p src/spatial/include/graffodil src/spatial/src
git mv src/batch/include/graffodil/QuadTree.h    src/spatial/include/graffodil/QuadTree.h
git mv src/batch/src/QuadTree.cpp                src/spatial/src/QuadTree.cpp
git mv src/batch/include/graffodil/SpatialGrid.h src/spatial/include/graffodil/SpatialGrid.h
```

- [x] **Step 3: Write the module CMakeLists**

`src/spatial/CMakeLists.txt`:

```cmake
add_library(graffodil-spatial STATIC)
add_library(Graffodil::Spatial ALIAS graffodil-spatial)
set_target_properties(graffodil-spatial PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(graffodil-spatial
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(graffodil-spatial PUBLIC Qt6::Core)
target_sources(graffodil-spatial PRIVATE
    src/QuadTree.cpp
    include/graffodil/QuadTree.h
    include/graffodil/SpatialGrid.h
)
```

- [x] **Step 4: Wire the other CMakeLists**

Root `CMakeLists.txt` — insert before the batch/force lines (order matters
only for readability; CMake resolves targets at generate time):

```cmake
add_subdirectory(src/core)
add_subdirectory(src/spatial)
add_subdirectory(src/sugiyama)
add_subdirectory(src/force)
add_subdirectory(src/circular)
add_subdirectory(src/batch)
```

`src/batch/CMakeLists.txt` — remove these three lines from `target_sources`:

```cmake
    src/QuadTree.cpp
    include/graffodil/QuadTree.h
    include/graffodil/SpatialGrid.h
```

(Batch keeps `target_link_libraries(graffodil-batch PUBLIC Graffodil::Core)`
unchanged — nothing in BatchRenderer/BatchNodeItem/BatchEdgeItem uses
QuadTree or SpatialGrid.)

`src/force/CMakeLists.txt` — change the link line:

```cmake
target_link_libraries(graffodil-force PUBLIC Graffodil::Core PRIVATE Graffodil::Spatial)
```

(was `... PRIVATE Graffodil::Batch`)

`tests/CMakeLists.txt` — retarget the two spatial tests:

```cmake
add_executable(tst_spatialgrid tst_spatialgrid.cpp)
target_link_libraries(tst_spatialgrid PRIVATE Graffodil::Spatial Qt6::Test)
add_test(NAME tst_spatialgrid COMMAND tst_spatialgrid)

add_executable(tst_quadtree tst_quadtree.cpp)
target_link_libraries(tst_quadtree PRIVATE Graffodil::Spatial Qt6::Test)
add_test(NAME tst_quadtree COMMAND tst_quadtree)
```

- [x] **Step 5: Clean-configure, build, full test suite**

A moved-file build needs a reconfigure:

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
(cd build && ctest --output-on-failure)
```

Expected: configure + build succeed; ALL tests pass (the force tests prove
QuadTree still links into Force via the new module).

Also verify Batch no longer claims the spatial headers:

```bash
grep -rn 'QuadTree\|SpatialGrid' src/batch/
```

Expected: no matches.

- [x] **Step 6: Commit**

```bash
git add -A src/ tests/CMakeLists.txt CMakeLists.txt
git commit -m "refactor(build): extract Graffodil::Spatial — Force no longer links the Batch renderer"
```

---

### Task 2: Update the docs that describe the module set

**Files:**
- Modify: `CLAUDE.md` (Architecture section)
- Modify: `docs/graffodil-design.md` (§Architecture / Boundary Map)
- Modify: `docs/ROADMAP.md`

- [x] **Step 1: CLAUDE.md** — the module list becomes:

```markdown
- `Graffodil::Core` — interfaces, scene, tools, edge rendering, utilities
- `Graffodil::Spatial` — QuadTree, SpatialGrid (depends: Qt6::Core only)
- `Graffodil::Sugiyama` — Sugiyama layered layout (depends: Core)
- `Graffodil::Force` — ForceAtlas2 + multilevel (depends: Core, Spatial)
- `Graffodil::Circular` — Circular layout (depends: Core)
- `Graffodil::Batch` — Batch rendering for 10K+ nodes (depends: Core)
```

- [x] **Step 2: graffodil-design.md** — in the Boundary Map / module sections,
move QuadTree+SpatialGrid under a new `Graffodil::Spatial` entry and correct
the Force dependency note (find the affected lines with
`grep -n 'QuadTree\|SpatialGrid\|Batch' docs/graffodil-design.md`).

- [x] **Step 3: ROADMAP.md** — Phase 5 → ☑ with commit hash; Status-log line.

- [x] **Step 4: Commit**

```bash
git add CLAUDE.md docs/
git commit -m "docs: mark Phase 5 (spatial module) complete"
```
