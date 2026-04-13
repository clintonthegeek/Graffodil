# Graffodil Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a shared Qt6 graph scene framework that PlanStan and Corbomite both consume, eliminating duplicated graph primitives (arrowheads, anchors, tools, edge rendering) across five graph drawing contexts.

**Architecture:** Core + Extensions model — one `Graffodil::Core` library (interfaces, scene, tools, edge rendering, utilities) with optional satellite targets (`Graffodil::Sugiyama`, `Graffodil::Force`, `Graffodil::Circular`, `Graffodil::Batch`). Clean-room interfaces, transplanted implementations from battle-tested code.

**Tech Stack:** C++20, Qt 6.8+, CMake 3.19+, QTest

---

## Reference Documents

- **Design spec:** `docs/graffodil-design.md`
- **Research survey:** `docs/research/00-landscape-survey.md`
- **PlanStan PERT report:** `docs/research/01-planstan-pert-graph.md`
- **PlanStan Gantt report:** `docs/research/02-planstan-gantt-arrows.md`
- **Corbomite ForceGraph report:** `docs/research/03-corbomite-forcegraph.md`
- **Corbomite Canvas report:** `docs/research/04-corbomite-canvas.md`

## Transplant Sources

When a task says "transplant from," it means: copy the source file, rename classes/namespaces to `Graffodil::`, adapt the interface to use `IGraphNode`/`IGraphEdge` instead of the original domain types, remove domain-specific code (CPM fields, Obsidian-specific logic), and verify the transplanted tests pass.

| Component | Source File | Approx LOC |
|-----------|------------|------------|
| Sugiyama layout | `~/dev/PlanStan/libs/graph/src/graphlayout.cpp` | 800 |
| ForceAtlas2 engine | `~/dev/Corbomite/libs/forcegraph/src/ForceLayoutEngine.cpp` | 600 |
| Multilevel layout | `~/dev/Corbomite/libs/forcegraph/src/MultilevelLayout.cpp` | 250 |
| QuadTree | `~/dev/Corbomite/libs/forcegraph/src/QuadTree.cpp` | 150 |
| SpatialGrid | `~/dev/Corbomite/libs/forcegraph/include/forcegraph/SpatialGrid.h` | 97 |
| BatchNodeItem | `~/dev/Corbomite/libs/forcegraph/src/BatchNodeItem.cpp` | 200 |
| BatchEdgeItem | `~/dev/Corbomite/libs/forcegraph/src/BatchEdgeItem.cpp` | 150 |
| Bezier edge path | `~/dev/Corbomite/libs/canvas/src/EdgeItem.cpp` lines 34-98 | 65 |
| Orthogonal edge path | `~/dev/PlanStan/libs/graph/src/dependencyedgeitem.cpp` | 200 |
| SelectMoveTool | `~/dev/PlanStan/libs/graph/src/selectmovetool.cpp` | 158 |
| CreateEdgeTool | `~/dev/PlanStan/libs/graph/src/createedgetool.cpp` | 158 |
| PanZoomTool | `~/dev/PlanStan/libs/graph/src/panzoomtool.cpp` | 103 |
| GraphMinimap | `~/dev/PlanStan/libs/graph/src/graphminimap.cpp` | 147 |
| ConnectorItem | `~/dev/PlanStan/libs/graph/src/connectoritem.cpp` | 36 |

## Execution Order

Tasks 1-11 are sequential. After that:
- **Task 13** (SpatialGrid + QuadTree) must come before **Task 12** (Force) — the Force module uses QuadTree from the Batch module.
- Task 14 (BatchRenderer) depends on Task 13.
- Task 15-16 are final.

## Build Commands

Throughout this plan, build and test with:

```bash
# Configure (first time or after adding files)
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build build

# Run all tests
cd build && ctest --output-on-failure

# Run a specific test
cd build && ctest -R tst_graphscene --output-on-failure
```

## File Map

### Graffodil::Core

| File | Responsibility |
|------|---------------|
| `src/core/include/graffodil/Types.h` | `Side`, `ArrowEnd` enums, `LayoutResult` struct |
| `src/core/include/graffodil/IGraphNode.h` | Node interface |
| `src/core/include/graffodil/IGraphEdge.h` | Edge interface |
| `src/core/include/graffodil/GraphScene.h` | Scene: node/edge registry, tool dispatch, selection, applyLayout |
| `src/core/src/GraphScene.cpp` | Scene implementation |
| `src/core/include/graffodil/EdgePathStrategy.h` | Path strategy interface + Direct/Bezier/Orthogonal/Polyline |
| `src/core/src/EdgePathStrategy.cpp` | Strategy implementations |
| `src/core/include/graffodil/TerminusStyle.h` | Terminus interface + Triangle/OpenArrow/Diamond/Circle/Bar/No |
| `src/core/src/TerminusStyle.cpp` | Terminus implementations |
| `src/core/include/graffodil/GraphEdgeItem.h` | Provided edge item composing strategy + terminus |
| `src/core/src/GraphEdgeItem.cpp` | Edge item implementation |
| `src/core/include/graffodil/GraphTool.h` | Tool base class |
| `src/core/src/GraphTool.cpp` | Tool base implementation |
| `src/core/include/graffodil/SelectMoveTool.h` | Select/move tool |
| `src/core/src/SelectMoveTool.cpp` | Transplanted from PlanStan + Corbomite |
| `src/core/include/graffodil/CreateEdgeTool.h` | Edge creation tool |
| `src/core/src/CreateEdgeTool.cpp` | Transplanted from PlanStan + Corbomite |
| `src/core/include/graffodil/PanZoomTool.h` | Pan/zoom tool |
| `src/core/src/PanZoomTool.cpp` | Transplanted from both codebases |
| `src/core/include/graffodil/AnchorHighlight.h` | Anchor visual feedback |
| `src/core/src/AnchorHighlight.cpp` | Highlight implementation |
| `src/core/include/graffodil/GraphMinimap.h` | Minimap overlay |
| `src/core/src/GraphMinimap.cpp` | Transplanted from PlanStan |
| `src/core/include/graffodil/ConnectorItem.h` | Waypoint dot |
| `src/core/src/ConnectorItem.cpp` | Transplanted from PlanStan |
| `src/core/include/graffodil/LayoutEngine.h` | Layout engine interface |

### Graffodil::Sugiyama

| File | Responsibility |
|------|---------------|
| `src/sugiyama/include/graffodil/SugiyamaLayout.h` | Sugiyama layout engine header |
| `src/sugiyama/src/SugiyamaLayout.cpp` | Transplanted from PlanStan graphlayout.cpp |

### Graffodil::Force

| File | Responsibility |
|------|---------------|
| `src/force/include/graffodil/ForceLayout.h` | ForceAtlas2 engine header |
| `src/force/src/ForceLayout.cpp` | Transplanted from Corbomite ForceLayoutEngine |
| `src/force/include/graffodil/MultilevelLayout.h` | Multilevel coarsening header |
| `src/force/src/MultilevelLayout.cpp` | Transplanted from Corbomite |

### Graffodil::Circular

| File | Responsibility |
|------|---------------|
| `src/circular/include/graffodil/CircularLayout.h` | Circular layout header |
| `src/circular/src/CircularLayout.cpp` | New implementation (~80 LOC) |

### Graffodil::Batch

| File | Responsibility |
|------|---------------|
| `src/batch/include/graffodil/SpatialGrid.h` | Header-only spatial index |
| `src/batch/include/graffodil/QuadTree.h` | Barnes-Hut quadtree header |
| `src/batch/src/QuadTree.cpp` | Transplanted from Corbomite |
| `src/batch/include/graffodil/BatchRenderer.h` | Batch rendering orchestrator |
| `src/batch/src/BatchRenderer.cpp` | Mode switching, synchronization |
| `src/batch/src/BatchNodeItem.h` | Internal batch node painter |
| `src/batch/src/BatchNodeItem.cpp` | Transplanted from Corbomite |
| `src/batch/src/BatchEdgeItem.h` | Internal batch edge painter |
| `src/batch/src/BatchEdgeItem.cpp` | Transplanted from Corbomite |

### Tests

| File | Covers |
|------|--------|
| `tests/tst_graphscene.cpp` | Scene registry, selection, applyLayout, tool dispatch |
| `tests/tst_edgepathstrategies.cpp` | All 4 path strategies |
| `tests/tst_terminus.cpp` | All 6 terminus styles |
| `tests/tst_graphedgeitem.cpp` | Edge item composition (strategy + terminus + adjust) |
| `tests/tst_tools.cpp` | SelectMove, CreateEdge, PanZoom tool event sequences |
| `tests/tst_sugiyama.cpp` | Sugiyama layout correctness (transplanted + new) |
| `tests/tst_forcelayout.cpp` | Force layout convergence (transplanted + new) |
| `tests/tst_circular.cpp` | Circular layout geometry |
| `tests/tst_quadtree.cpp` | QuadTree correctness (transplanted) |
| `tests/tst_spatialgrid.cpp` | SpatialGrid queries |
| `tests/tst_batchrenderer.cpp` | Batch mode switching, synchronization |

---

### Task 1: Project Skeleton & CMake

**Files:**
- Create: `CMakeLists.txt`
- Create: `src/core/CMakeLists.txt`
- Create: `src/sugiyama/CMakeLists.txt`
- Create: `src/force/CMakeLists.txt`
- Create: `src/circular/CMakeLists.txt`
- Create: `src/batch/CMakeLists.txt`
- Create: `tests/CMakeLists.txt`
- Create: `.gitignore`
- Create: `.clangd`

- [ ] **Step 1: Create top-level CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.19)
project(Graffodil VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Widgets Test)

add_subdirectory(src/core)
add_subdirectory(src/sugiyama)
add_subdirectory(src/force)
add_subdirectory(src/circular)
add_subdirectory(src/batch)

enable_testing()
add_subdirectory(tests)
```

- [ ] **Step 2: Create Core CMakeLists.txt**

```cmake
# src/core/CMakeLists.txt
add_library(graffodil-core STATIC)
add_library(Graffodil::Core ALIAS graffodil-core)

set_target_properties(graffodil-core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    AUTOMOC ON
)

target_include_directories(graffodil-core
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(graffodil-core PUBLIC Qt6::Core Qt6::Widgets)

# Sources added incrementally in later tasks
```

- [ ] **Step 3: Create extension module CMakeLists.txt stubs**

`src/sugiyama/CMakeLists.txt`:
```cmake
add_library(graffodil-sugiyama STATIC)
add_library(Graffodil::Sugiyama ALIAS graffodil-sugiyama)
set_target_properties(graffodil-sugiyama PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)
target_include_directories(graffodil-sugiyama
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(graffodil-sugiyama PUBLIC Graffodil::Core)
```

`src/force/CMakeLists.txt`:
```cmake
add_library(graffodil-force STATIC)
add_library(Graffodil::Force ALIAS graffodil-force)
set_target_properties(graffodil-force PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)
target_include_directories(graffodil-force
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(graffodil-force PUBLIC Graffodil::Core)
```

`src/circular/CMakeLists.txt`:
```cmake
add_library(graffodil-circular STATIC)
add_library(Graffodil::Circular ALIAS graffodil-circular)
set_target_properties(graffodil-circular PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)
target_include_directories(graffodil-circular
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(graffodil-circular PUBLIC Graffodil::Core)
```

`src/batch/CMakeLists.txt`:
```cmake
add_library(graffodil-batch STATIC)
add_library(Graffodil::Batch ALIAS graffodil-batch)
set_target_properties(graffodil-batch PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)
target_include_directories(graffodil-batch
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(graffodil-batch PUBLIC Graffodil::Core)
```

- [ ] **Step 4: Create tests CMakeLists.txt stub**

```cmake
# tests/CMakeLists.txt
# Test targets added incrementally as test files are created.
```

- [ ] **Step 5: Create .gitignore and .clangd**

`.gitignore`:
```
build/
compile_commands.json
.cache/
```

`.clangd`:
```
CompilationDatabase: build
```

- [ ] **Step 6: Create placeholder source files so the build succeeds**

Each library target needs at least one source file or CMake will error. Create minimal placeholders:

`src/core/src/placeholder.cpp`:
```cpp
// Graffodil::Core — placeholder removed once real sources are added
```

`src/sugiyama/src/placeholder.cpp`:
```cpp
// Graffodil::Sugiyama — placeholder removed once real sources are added
```

`src/force/src/placeholder.cpp`:
```cpp
// Graffodil::Force — placeholder removed once real sources are added
```

`src/circular/src/placeholder.cpp`:
```cpp
// Graffodil::Circular — placeholder removed once real sources are added
```

`src/batch/src/placeholder.cpp`:
```cpp
// Graffodil::Batch — placeholder removed once real sources are added
```

Add `src/placeholder.cpp` to each target's `target_sources()`:

Append to each module's `CMakeLists.txt`:
```cmake
target_sources(<target-name> PRIVATE src/placeholder.cpp)
```

For core: `target_sources(graffodil-core PRIVATE src/placeholder.cpp)`
For sugiyama: `target_sources(graffodil-sugiyama PRIVATE src/placeholder.cpp)`
(etc. for force, circular, batch)

- [ ] **Step 7: Verify build**

Run:
```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build
```
Expected: clean build, zero errors, five static libraries produced.

- [ ] **Step 8: Create compile_commands.json symlink**

```bash
ln -sf build/compile_commands.json compile_commands.json
```

- [ ] **Step 9: Commit**

```bash
git add -A
git commit -m "feat: project skeleton — CMake targets for Core + 4 extension modules"
```

---

### Task 2: Core Interfaces & Types

**Files:**
- Create: `src/core/include/graffodil/Types.h`
- Create: `src/core/include/graffodil/IGraphNode.h`
- Create: `src/core/include/graffodil/IGraphEdge.h`
- Create: `src/core/include/graffodil/LayoutEngine.h`
- Test: `tests/tst_graphscene.cpp` (first test — interfaces compile and work)

- [ ] **Step 1: Write Types.h**

```cpp
#pragma once

#include <QHash>
#include <QList>
#include <QPointF>
#include <QPair>
#include <QString>

namespace Graffodil {

enum class Side { Top, Right, Bottom, Left };

enum class ArrowEnd { Source, Target };

/// Communication contract between layout engines and the scene.
/// Layout engines produce this; the scene consumes it via applyLayout().
struct LayoutResult {
    QHash<QString, QPointF> nodePositions;
    /// Optional waypoints for edges spanning multiple layers (Sugiyama)
    /// or needing routing hints. Key is (sourceId, targetId).
    QHash<QPair<QString, QString>, QList<QPointF>> edgeWaypoints;
};

} // namespace Graffodil
```

- [ ] **Step 2: Write IGraphNode.h**

```cpp
#pragma once

#include <graffodil/Types.h>
#include <QGraphicsItem>
#include <QPointF>
#include <QRectF>
#include <QString>

namespace Graffodil {

/// Anything that can participate as a node in a GraphScene.
/// Consumers implement this on their own QGraphicsItem subclasses.
class IGraphNode {
public:
    virtual ~IGraphNode() = default;

    /// Unique identifier for this node within the scene.
    virtual QString nodeId() const = 0;

    /// Connection point for the given side. Used by edges and tools
    /// to determine where to attach/draw from.
    virtual QPointF anchorPoint(Side side) const = 0;

    /// Bounding rect in scene coordinates. Used by the scene for
    /// spatial queries and by layout engines for spacing.
    virtual QRectF nodeBoundingRect() const = 0;

    /// Bridge to the Qt graphics item system. The scene calls this
    /// to add/remove the item from the QGraphicsScene.
    /// Must return a stable pointer for the lifetime of the node.
    virtual QGraphicsItem *graphicsItem() = 0;
};

} // namespace Graffodil
```

- [ ] **Step 3: Write IGraphEdge.h**

```cpp
#pragma once

#include <QGraphicsItem>
#include <QString>

namespace Graffodil {

class IGraphNode;

/// Anything that can participate as an edge in a GraphScene.
/// Consumers can use the provided GraphEdgeItem or implement their own.
class IGraphEdge {
public:
    virtual ~IGraphEdge() = default;

    virtual QString edgeId() const = 0;
    virtual IGraphNode *sourceNode() const = 0;
    virtual IGraphNode *targetNode() const = 0;

    /// Bridge to QGraphicsItem, same pattern as IGraphNode.
    virtual QGraphicsItem *graphicsItem() = 0;
};

} // namespace Graffodil
```

- [ ] **Step 4: Write LayoutEngine.h**

```cpp
#pragma once

#include <graffodil/Types.h>
#include <QList>

namespace Graffodil {

class IGraphNode;
class IGraphEdge;

/// Base class for layout algorithms. Produces a LayoutResult
/// that the consumer applies to the scene via scene->applyLayout().
/// The engine does not touch scene items directly.
class LayoutEngine {
public:
    virtual ~LayoutEngine() = default;

    virtual void setNodes(const QList<IGraphNode *> &nodes) = 0;
    virtual void setEdges(const QList<IGraphEdge *> &edges) = 0;

    /// Compute positions in one shot. For iterative layouts, runs
    /// the full simulation to convergence.
    virtual LayoutResult compute() = 0;

    /// For iterative layouts: run one simulation step.
    /// Returns false when converged.
    virtual bool tick() { return false; }

    /// Whether this engine is iterative (force-directed) or one-shot (Sugiyama).
    virtual bool isIterative() const { return false; }

    /// Current positions after the last tick(). Only meaningful for
    /// iterative layouts. One-shot layouts return empty until compute().
    virtual LayoutResult currentPositions() const { return {}; }
};

} // namespace Graffodil
```

- [ ] **Step 5: Write a test helper — TestNode (reused by many tests)**

Create `tests/TestHelpers.h`:

```cpp
#pragma once

#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include <QGraphicsRectItem>

namespace Graffodil::Test {

/// Minimal IGraphNode for testing. A 100x60 rectangle.
class TestNode : public QGraphicsRectItem, public IGraphNode {
public:
    explicit TestNode(const QString &id, const QPointF &pos = {})
        : QGraphicsRectItem(0, 0, 100, 60), m_id(id)
    {
        setPos(pos);
    }

    QString nodeId() const override { return m_id; }

    QPointF anchorPoint(Side side) const override {
        const QRectF r = sceneBoundingRect();
        switch (side) {
        case Side::Top:    return {r.center().x(), r.top()};
        case Side::Bottom: return {r.center().x(), r.bottom()};
        case Side::Left:   return {r.left(), r.center().y()};
        case Side::Right:  return {r.right(), r.center().y()};
        }
        Q_UNREACHABLE();
    }

    QRectF nodeBoundingRect() const override { return sceneBoundingRect(); }
    QGraphicsItem *graphicsItem() override { return this; }

private:
    QString m_id;
};

/// Minimal IGraphEdge for testing.
class TestEdge : public QGraphicsLineItem, public IGraphEdge {
public:
    TestEdge(const QString &id, IGraphNode *source, IGraphNode *target)
        : m_id(id), m_source(source), m_target(target) {}

    QString edgeId() const override { return m_id; }
    IGraphNode *sourceNode() const override { return m_source; }
    IGraphNode *targetNode() const override { return m_target; }
    QGraphicsItem *graphicsItem() override { return this; }

private:
    QString m_id;
    IGraphNode *m_source;
    IGraphNode *m_target;
};

} // namespace Graffodil::Test
```

- [ ] **Step 6: Write initial tst_graphscene.cpp — interface compilation test**

```cpp
#include <QTest>
#include "TestHelpers.h"

using namespace Graffodil;
using namespace Graffodil::Test;

class TestInterfaces : public QObject {
    Q_OBJECT
private slots:
    void testNodeInterface() {
        TestNode node("n1", {10, 20});
        QCOMPARE(node.nodeId(), "n1");
        QCOMPARE(node.anchorPoint(Side::Top).x(), 60.0);  // 10 + 100/2
        QCOMPARE(node.anchorPoint(Side::Top).y(), 20.0);
        QCOMPARE(node.anchorPoint(Side::Right).x(), 110.0); // 10 + 100
        QVERIFY(node.graphicsItem() != nullptr);
    }

    void testEdgeInterface() {
        TestNode src("src"), tgt("tgt");
        TestEdge edge("e1", &src, &tgt);
        QCOMPARE(edge.edgeId(), "e1");
        QCOMPARE(edge.sourceNode()->nodeId(), "src");
        QCOMPARE(edge.targetNode()->nodeId(), "tgt");
        QVERIFY(edge.graphicsItem() != nullptr);
    }

    void testLayoutResult() {
        LayoutResult result;
        result.nodePositions["n1"] = {100, 200};
        QCOMPARE(result.nodePositions.size(), 1);
        QCOMPARE(result.nodePositions["n1"], QPointF(100, 200));
    }
};

QTEST_MAIN(TestInterfaces)
#include "tst_graphscene.moc"
```

- [ ] **Step 7: Add test to CMake**

Append to `tests/CMakeLists.txt`:
```cmake
add_executable(tst_graphscene tst_graphscene.cpp)
target_link_libraries(tst_graphscene PRIVATE Graffodil::Core Qt6::Test)
target_include_directories(tst_graphscene PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_graphscene COMMAND tst_graphscene)
```

- [ ] **Step 8: Build and run test**

```bash
cmake --build build && cd build && ctest -R tst_graphscene --output-on-failure
```
Expected: PASS (3 test functions)

- [ ] **Step 9: Commit**

```bash
git add -A
git commit -m "feat(core): interfaces — IGraphNode, IGraphEdge, LayoutEngine, Types"
```

---

### Task 3: GraphScene

**Files:**
- Create: `src/core/include/graffodil/GraphScene.h`
- Create: `src/core/src/GraphScene.cpp`
- Modify: `src/core/CMakeLists.txt` (add sources)
- Modify: `tests/tst_graphscene.cpp` (add scene tests)

- [ ] **Step 1: Write the failing tests**

Add to `tests/tst_graphscene.cpp` (add `#include <graffodil/GraphScene.h>` at top):

```cpp
    void testAddRemoveNodes() {
        GraphScene scene;
        auto *n1 = new TestNode("n1");
        auto *n2 = new TestNode("n2");

        scene.addNode(n1);
        scene.addNode(n2);
        QCOMPARE(scene.nodes().size(), 2);
        QCOMPARE(scene.nodeForId("n1"), n1);
        QCOMPARE(scene.nodeForId("n2"), n2);
        QVERIFY(scene.nodeForId("n3") == nullptr);

        scene.removeNode("n1");
        QCOMPARE(scene.nodes().size(), 1);
        QVERIFY(scene.nodeForId("n1") == nullptr);
    }

    void testAddRemoveEdges() {
        GraphScene scene;
        auto *n1 = new TestNode("n1");
        auto *n2 = new TestNode("n2");
        scene.addNode(n1);
        scene.addNode(n2);

        auto *e1 = new TestEdge("e1", n1, n2);
        scene.addEdge(e1);
        QCOMPARE(scene.edges().size(), 1);
        QCOMPARE(scene.edgeForId("e1"), e1);
        QCOMPARE(scene.edgeBetween("n1", "n2"), e1);
        QVERIFY(scene.edgeBetween("n2", "n1") == nullptr);

        scene.removeEdge("e1");
        QCOMPARE(scene.edges().size(), 0);
    }

    void testNodeAddedSignal() {
        GraphScene scene;
        QSignalSpy spy(&scene, &GraphScene::nodeAdded);
        auto *n1 = new TestNode("n1");
        scene.addNode(n1);
        QCOMPARE(spy.count(), 1);
    }

    void testApplyLayout() {
        GraphScene scene;
        auto *n1 = new TestNode("n1", {0, 0});
        auto *n2 = new TestNode("n2", {0, 0});
        scene.addNode(n1);
        scene.addNode(n2);

        LayoutResult result;
        result.nodePositions["n1"] = {100, 200};
        result.nodePositions["n2"] = {300, 400};
        scene.applyLayout(result);

        QCOMPARE(n1->graphicsItem()->pos(), QPointF(100, 200));
        QCOMPARE(n2->graphicsItem()->pos(), QPointF(300, 400));
    }

    void testSelectedNodes() {
        GraphScene scene;
        auto *n1 = new TestNode("n1");
        auto *n2 = new TestNode("n2");
        n1->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
        n2->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
        scene.addNode(n1);
        scene.addNode(n2);

        n1->graphicsItem()->setSelected(true);
        QCOMPARE(scene.selectedNodes().size(), 1);
        QCOMPARE(scene.selectedNodes().first(), n1);
    }
```

Also add `#include <QSignalSpy>` at the top.

- [ ] **Step 2: Run tests — verify they fail**

```bash
cmake --build build 2>&1 | head -5
```
Expected: compilation error — `GraphScene.h` not found.

- [ ] **Step 3: Write GraphScene.h**

```cpp
#pragma once

#include <graffodil/Types.h>
#include <QGraphicsScene>
#include <QHash>

namespace Graffodil {

class IGraphNode;
class IGraphEdge;
class GraphTool;

class GraphScene : public QGraphicsScene {
    Q_OBJECT
public:
    using QGraphicsScene::QGraphicsScene;

    // --- Node/edge registry ---

    void addNode(IGraphNode *node);
    void removeNode(const QString &id);
    IGraphNode *nodeForId(const QString &id) const;
    QList<IGraphNode *> nodes() const;

    void addEdge(IGraphEdge *edge);
    void removeEdge(const QString &id);
    IGraphEdge *edgeForId(const QString &id) const;
    IGraphEdge *edgeBetween(const QString &sourceId, const QString &targetId) const;
    QList<IGraphEdge *> edges() const;

    // --- Tool dispatch ---
    // Consumer wires tool activation to QAction/KAction.
    // The scene doesn't know about action frameworks.
    void setActiveTool(GraphTool *tool);
    GraphTool *activeTool() const;

    // --- Selection (typed over interfaces) ---
    QList<IGraphNode *> selectedNodes() const;
    QList<IGraphEdge *> selectedEdges() const;

    // --- Layout ---
    // Applies computed positions to registered nodes.
    // For edges: if edgeWaypoints are provided and the edge is a
    // GraphEdgeItem, waypoints are passed to the path strategy.
    void applyLayout(const LayoutResult &result);

signals:
    void nodeAdded(IGraphNode *node);
    void nodeRemoved(const QString &id);
    void edgeAdded(IGraphEdge *edge);
    void edgeRemoved(const QString &id);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    QHash<QString, IGraphNode *> m_nodes;
    QHash<QString, IGraphEdge *> m_edges;
    GraphTool *m_activeTool = nullptr;
};

} // namespace Graffodil
```

- [ ] **Step 4: Write GraphScene.cpp**

```cpp
#include <graffodil/GraphScene.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>

namespace Graffodil {

void GraphScene::addNode(IGraphNode *node)
{
    if (!node || m_nodes.contains(node->nodeId()))
        return;
    m_nodes.insert(node->nodeId(), node);
    addItem(node->graphicsItem());
    emit nodeAdded(node);
}

void GraphScene::removeNode(const QString &id)
{
    auto it = m_nodes.find(id);
    if (it == m_nodes.end())
        return;
    IGraphNode *node = it.value();
    removeItem(node->graphicsItem());
    m_nodes.erase(it);
    emit nodeRemoved(id);
}

IGraphNode *GraphScene::nodeForId(const QString &id) const
{
    return m_nodes.value(id, nullptr);
}

QList<IGraphNode *> GraphScene::nodes() const
{
    return m_nodes.values();
}

void GraphScene::addEdge(IGraphEdge *edge)
{
    if (!edge || m_edges.contains(edge->edgeId()))
        return;
    m_edges.insert(edge->edgeId(), edge);
    addItem(edge->graphicsItem());
    emit edgeAdded(edge);
}

void GraphScene::removeEdge(const QString &id)
{
    auto it = m_edges.find(id);
    if (it == m_edges.end())
        return;
    IGraphEdge *edge = it.value();
    removeItem(edge->graphicsItem());
    m_edges.erase(it);
    emit edgeRemoved(id);
}

IGraphEdge *GraphScene::edgeForId(const QString &id) const
{
    return m_edges.value(id, nullptr);
}

IGraphEdge *GraphScene::edgeBetween(const QString &sourceId, const QString &targetId) const
{
    for (auto *edge : m_edges) {
        if (edge->sourceNode()->nodeId() == sourceId
            && edge->targetNode()->nodeId() == targetId) {
            return edge;
        }
    }
    return nullptr;
}

QList<IGraphEdge *> GraphScene::edges() const
{
    return m_edges.values();
}

void GraphScene::setActiveTool(GraphTool *tool)
{
    m_activeTool = tool;
}

GraphTool *GraphScene::activeTool() const
{
    return m_activeTool;
}

QList<IGraphNode *> GraphScene::selectedNodes() const
{
    QList<IGraphNode *> result;
    for (auto *node : m_nodes) {
        if (node->graphicsItem()->isSelected())
            result.append(node);
    }
    return result;
}

QList<IGraphEdge *> GraphScene::selectedEdges() const
{
    QList<IGraphEdge *> result;
    for (auto *edge : m_edges) {
        if (edge->graphicsItem()->isSelected())
            result.append(edge);
    }
    return result;
}

void GraphScene::applyLayout(const LayoutResult &result)
{
    for (auto it = result.nodePositions.constBegin();
         it != result.nodePositions.constEnd(); ++it) {
        if (auto *node = nodeForId(it.key()))
            node->graphicsItem()->setPos(it.value());
    }
}

// Tool dispatch — forward events to active tool if set.
// These are the extension points for interaction.
void GraphScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    QGraphicsScene::mousePressEvent(e);
}

void GraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    QGraphicsScene::mouseMoveEvent(e);
}

void GraphScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    QGraphicsScene::mouseReleaseEvent(e);
}

void GraphScene::keyPressEvent(QKeyEvent *e)
{
    QGraphicsScene::keyPressEvent(e);
}

} // namespace Graffodil
```

- [ ] **Step 5: Update Core CMakeLists.txt**

Replace the placeholder source with real sources in `src/core/CMakeLists.txt`:
```cmake
target_sources(graffodil-core PRIVATE
    src/GraphScene.cpp
    include/graffodil/Types.h
    include/graffodil/IGraphNode.h
    include/graffodil/IGraphEdge.h
    include/graffodil/LayoutEngine.h
    include/graffodil/GraphScene.h
)
```

Remove `src/placeholder.cpp` and its `target_sources` line.

- [ ] **Step 6: Build and run tests**

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build && cd build && ctest -R tst_graphscene --output-on-failure
```
Expected: all 8 tests PASS.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat(core): GraphScene — node/edge registry, selection, applyLayout"
```

---

### Task 4: Edge Path Strategies

**Files:**
- Create: `src/core/include/graffodil/EdgePathStrategy.h`
- Create: `src/core/src/EdgePathStrategy.cpp`
- Create: `tests/tst_edgepathstrategies.cpp`
- Modify: `src/core/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing tests**

`tests/tst_edgepathstrategies.cpp`:
```cpp
#include <QTest>
#include <graffodil/EdgePathStrategy.h>

using namespace Graffodil;

class TestEdgePathStrategies : public QObject {
    Q_OBJECT
private slots:
    void directPath() {
        DirectPathStrategy strategy;
        QPointF src(0, 0), tgt(100, 0);
        auto path = strategy.computePath(src, Side::Right, tgt, Side::Left);
        QVERIFY(!path.isEmpty());
        QCOMPARE(path.elementAt(0).x, 0.0);
        QCOMPARE(path.elementAt(0).y, 0.0);
        // Direct path ends at target
        auto last = path.elementAt(path.elementCount() - 1);
        QCOMPARE(last.x, 100.0);
        QCOMPARE(last.y, 0.0);
    }

    void bezierPath() {
        BezierPathStrategy strategy;
        QPointF src(0, 0), tgt(200, 0);
        auto path = strategy.computePath(src, Side::Right, tgt, Side::Left);
        QVERIFY(!path.isEmpty());
        // Bezier path has cubic elements (not just lineTo)
        bool hasCubic = false;
        for (int i = 0; i < path.elementCount(); ++i) {
            if (path.elementAt(i).type == QPainterPath::CurveToElement)
                hasCubic = true;
        }
        QVERIFY(hasCubic);
    }

    void bezierCurvatureCapped() {
        BezierPathStrategy strategy;
        // Very far apart — curvature should cap at 80.0
        QPointF src(0, 0), tgt(1000, 0);
        auto path = strategy.computePath(src, Side::Right, tgt, Side::Left);
        // Control point should be at most 80px from source
        // Element 1 is the first control point of cubicTo
        auto ctrl1 = path.elementAt(1);
        QVERIFY(ctrl1.x <= 80.0 + 0.1);
    }

    void orthogonalPath() {
        OrthogonalPathStrategy strategy;
        QPointF src(0, 50), tgt(200, 150);
        auto path = strategy.computePath(src, Side::Right, tgt, Side::Left);
        QVERIFY(!path.isEmpty());
        // Orthogonal: all segments are axis-aligned
        for (int i = 1; i < path.elementCount(); ++i) {
            auto prev = path.elementAt(i - 1);
            auto cur = path.elementAt(i);
            if (cur.type == QPainterPath::LineToElement) {
                bool horizontal = qFuzzyCompare(prev.y, cur.y);
                bool vertical = qFuzzyCompare(prev.x, cur.x);
                QVERIFY2(horizontal || vertical,
                    qPrintable(QString("Segment %1 is diagonal: (%2,%3)->(%4,%5)")
                        .arg(i).arg(prev.x).arg(prev.y).arg(cur.x).arg(cur.y)));
            }
        }
    }

    void polylineThroughWaypoints() {
        PolylinePathStrategy strategy;
        QPointF src(0, 0), tgt(300, 0);
        QList<QPointF> waypoints = {{100, 50}, {200, 50}};
        auto path = strategy.computePath(src, Side::Right, tgt, Side::Left, waypoints);
        QVERIFY(!path.isEmpty());
        // Should pass through waypoints
        bool found100 = false, found200 = false;
        for (int i = 0; i < path.elementCount(); ++i) {
            auto e = path.elementAt(i);
            if (qFuzzyCompare(e.x, 100.0) && qFuzzyCompare(e.y, 50.0))
                found100 = true;
            if (qFuzzyCompare(e.x, 200.0) && qFuzzyCompare(e.y, 50.0))
                found200 = true;
        }
        QVERIFY(found100);
        QVERIFY(found200);
    }
};

QTEST_MAIN(TestEdgePathStrategies)
#include "tst_edgepathstrategies.moc"
```

- [ ] **Step 2: Run tests — verify compilation fails**

Expected: `EdgePathStrategy.h` not found.

- [ ] **Step 3: Write EdgePathStrategy.h**

```cpp
#pragma once

#include <graffodil/Types.h>
#include <QPainterPath>
#include <QList>
#include <QPointF>

namespace Graffodil {

/// Pluggable path computation for edges. Determines the shape of the
/// line between two anchor points. Subclass for custom routing.
class EdgePathStrategy {
public:
    virtual ~EdgePathStrategy() = default;

    /// Compute the QPainterPath for an edge given its endpoints and
    /// optional waypoints (from layout engines or manual routing).
    virtual QPainterPath computePath(
        const QPointF &sourceAnchor, Side sourceSide,
        const QPointF &targetAnchor, Side targetSide,
        const QList<QPointF> &waypoints = {}) = 0;
};

/// Straight line from source to target. Ignores waypoints.
class DirectPathStrategy : public EdgePathStrategy {
public:
    QPainterPath computePath(
        const QPointF &sourceAnchor, Side sourceSide,
        const QPointF &targetAnchor, Side targetSide,
        const QList<QPointF> &waypoints = {}) override;
};

/// Cubic bezier with S-shaped curves.
/// Curvature = min(distance * 0.4, 80.0). Control points extend
/// outward from the connection side.
/// Transplanted from Corbomite libs/canvas/src/EdgeItem.cpp.
class BezierPathStrategy : public EdgePathStrategy {
public:
    QPainterPath computePath(
        const QPointF &sourceAnchor, Side sourceSide,
        const QPointF &targetAnchor, Side targetSide,
        const QList<QPointF> &waypoints = {}) override;
};

/// Right-angle segments with waypoint support.
/// Transplanted from PlanStan libs/graph/src/dependencyedgeitem.cpp.
class OrthogonalPathStrategy : public EdgePathStrategy {
public:
    QPainterPath computePath(
        const QPointF &sourceAnchor, Side sourceSide,
        const QPointF &targetAnchor, Side targetSide,
        const QList<QPointF> &waypoints = {}) override;
};

/// Multi-segment straight lines through waypoints.
class PolylinePathStrategy : public EdgePathStrategy {
public:
    QPainterPath computePath(
        const QPointF &sourceAnchor, Side sourceSide,
        const QPointF &targetAnchor, Side targetSide,
        const QList<QPointF> &waypoints = {}) override;
};

} // namespace Graffodil
```

- [ ] **Step 4: Write EdgePathStrategy.cpp**

```cpp
#include <graffodil/EdgePathStrategy.h>
#include <cmath>

namespace Graffodil {

// --- DirectPathStrategy ---

QPainterPath DirectPathStrategy::computePath(
    const QPointF &sourceAnchor, Side /*sourceSide*/,
    const QPointF &targetAnchor, Side /*targetSide*/,
    const QList<QPointF> &/*waypoints*/)
{
    QPainterPath path;
    path.moveTo(sourceAnchor);
    path.lineTo(targetAnchor);
    return path;
}

// --- BezierPathStrategy ---
// Transplanted from Corbomite libs/canvas/src/EdgeItem.cpp lines 34-83.
// Curvature formula: min(distance * 0.4, 80.0).
// Control points extend outward from connection side.

static QPointF controlPointOffset(Side side, double curvature)
{
    switch (side) {
    case Side::Right:  return {curvature, 0};
    case Side::Left:   return {-curvature, 0};
    case Side::Bottom: return {0, curvature};
    case Side::Top:    return {0, -curvature};
    }
    Q_UNREACHABLE();
}

QPainterPath BezierPathStrategy::computePath(
    const QPointF &sourceAnchor, Side sourceSide,
    const QPointF &targetAnchor, Side targetSide,
    const QList<QPointF> &/*waypoints*/)
{
    const double dx = targetAnchor.x() - sourceAnchor.x();
    const double dy = targetAnchor.y() - sourceAnchor.y();
    const double dist = std::sqrt(dx * dx + dy * dy);
    const double curvature = std::min(dist * 0.4, 80.0);

    const QPointF ctrl1 = sourceAnchor + controlPointOffset(sourceSide, curvature);
    const QPointF ctrl2 = targetAnchor + controlPointOffset(targetSide, curvature);

    QPainterPath path;
    path.moveTo(sourceAnchor);
    path.cubicTo(ctrl1, ctrl2, targetAnchor);
    return path;
}

// --- OrthogonalPathStrategy ---
// Transplanted from PlanStan libs/graph/src/dependencyedgeitem.cpp.
// Produces right-angle segments. Uses waypoints if provided.

static constexpr double kOrthogonalGap = 20.0;

QPainterPath OrthogonalPathStrategy::computePath(
    const QPointF &sourceAnchor, Side sourceSide,
    const QPointF &targetAnchor, Side targetSide,
    const QList<QPointF> &waypoints)
{
    QPainterPath path;
    path.moveTo(sourceAnchor);

    if (!waypoints.isEmpty()) {
        // Route through provided waypoints with axis-aligned segments
        QPointF prev = sourceAnchor;
        for (const auto &wp : waypoints) {
            // Horizontal then vertical
            path.lineTo(wp.x(), prev.y());
            path.lineTo(wp.x(), wp.y());
            prev = wp;
        }
        path.lineTo(targetAnchor.x(), prev.y());
        path.lineTo(targetAnchor);
    } else {
        // Simple orthogonal: extend from source side, bend, arrive at target side
        QPointF exitPt = sourceAnchor + controlPointOffset(sourceSide, kOrthogonalGap);
        QPointF entryPt = targetAnchor + controlPointOffset(targetSide, kOrthogonalGap);

        path.lineTo(exitPt);

        // Connect exit to entry with one or two bends
        if (sourceSide == Side::Right || sourceSide == Side::Left) {
            // Source exits horizontally
            if (targetSide == Side::Left || targetSide == Side::Right) {
                // Target also horizontal — use midpoint X
                double midX = (exitPt.x() + entryPt.x()) / 2.0;
                path.lineTo(midX, exitPt.y());
                path.lineTo(midX, entryPt.y());
            } else {
                // Target vertical — L-bend
                path.lineTo(entryPt.x(), exitPt.y());
            }
        } else {
            // Source exits vertically
            if (targetSide == Side::Top || targetSide == Side::Bottom) {
                double midY = (exitPt.y() + entryPt.y()) / 2.0;
                path.lineTo(exitPt.x(), midY);
                path.lineTo(entryPt.x(), midY);
            } else {
                path.lineTo(exitPt.x(), entryPt.y());
            }
        }

        path.lineTo(entryPt);
        path.lineTo(targetAnchor);
    }
    return path;
}

// --- PolylinePathStrategy ---

QPainterPath PolylinePathStrategy::computePath(
    const QPointF &sourceAnchor, Side /*sourceSide*/,
    const QPointF &targetAnchor, Side /*targetSide*/,
    const QList<QPointF> &waypoints)
{
    QPainterPath path;
    path.moveTo(sourceAnchor);
    for (const auto &wp : waypoints)
        path.lineTo(wp);
    path.lineTo(targetAnchor);
    return path;
}

} // namespace Graffodil
```

- [ ] **Step 5: Add sources to Core CMakeLists.txt**

Add to the `target_sources(graffodil-core ...)` block:
```cmake
    src/EdgePathStrategy.cpp
    include/graffodil/EdgePathStrategy.h
```

- [ ] **Step 6: Add test to tests/CMakeLists.txt**

```cmake
add_executable(tst_edgepathstrategies tst_edgepathstrategies.cpp)
target_link_libraries(tst_edgepathstrategies PRIVATE Graffodil::Core Qt6::Test)
add_test(NAME tst_edgepathstrategies COMMAND tst_edgepathstrategies)
```

- [ ] **Step 7: Build and run tests**

```bash
cmake --build build && cd build && ctest -R tst_edgepathstrategies --output-on-failure
```
Expected: all 5 tests PASS.

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat(core): edge path strategies — Direct, Bezier, Orthogonal, Polyline"
```

---

### Task 5: Terminus Styles

**Files:**
- Create: `src/core/include/graffodil/TerminusStyle.h`
- Create: `src/core/src/TerminusStyle.cpp`
- Create: `tests/tst_terminus.cpp`
- Modify: `src/core/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing tests**

`tests/tst_terminus.cpp`:
```cpp
#include <QTest>
#include <QPainter>
#include <QImage>
#include <graffodil/TerminusStyle.h>

using namespace Graffodil;

class TestTerminus : public QObject {
    Q_OBJECT
private slots:
    void triangleInset() {
        TriangleTerminus term;
        QImage img(100, 100, QImage::Format_ARGB32);
        QPainter painter(&img);
        QPen pen(Qt::black, 2);

        // Arrow pointing right: tip at (80,50), arriving from (20,50)
        QPointF inset = term.draw(&painter, {80, 50}, {20, 50}, pen);
        // Inset should be to the left of the tip
        QVERIFY(inset.x() < 80.0);
        QCOMPARE(inset.y(), 50.0);  // same Y — horizontal arrow
    }

    void noTerminusPassthrough() {
        NoTerminus term;
        QImage img(100, 100, QImage::Format_ARGB32);
        QPainter painter(&img);
        QPen pen(Qt::black, 2);

        QPointF tip(80, 50);
        QPointF inset = term.draw(&painter, tip, {20, 50}, pen);
        // NoTerminus returns tip unchanged
        QCOMPARE(inset, tip);
    }

    void terminusSizePositive() {
        TriangleTerminus triangle;
        OpenArrowTerminus openArrow;
        DiamondTerminus diamond;
        CircleTerminus circle;
        BarTerminus bar;
        NoTerminus none;

        QVERIFY(triangle.size() > 0);
        QVERIFY(openArrow.size() > 0);
        QVERIFY(diamond.size() > 0);
        QVERIFY(circle.size() > 0);
        QVERIFY(bar.size() > 0);
        QCOMPARE(none.size(), 0.0);
    }

    void diamondFillToggle() {
        DiamondTerminus filled(true);
        DiamondTerminus unfilled(false);
        // Both should produce valid inset points
        QImage img(100, 100, QImage::Format_ARGB32);
        QPainter painter(&img);
        QPen pen(Qt::black, 2);

        QPointF inset1 = filled.draw(&painter, {80, 50}, {20, 50}, pen);
        QPointF inset2 = unfilled.draw(&painter, {80, 50}, {20, 50}, pen);
        QVERIFY(inset1.x() < 80.0);
        QVERIFY(inset2.x() < 80.0);
    }
};

QTEST_MAIN(TestTerminus)
#include "tst_terminus.moc"
```

- [ ] **Step 2: Run tests — verify compilation fails**

- [ ] **Step 3: Write TerminusStyle.h**

```cpp
#pragma once

#include <QPainter>
#include <QPointF>
#include <QPen>

namespace Graffodil {

/// Pluggable endpoint rendering for edges. Subclass for custom shapes
/// (e.g., CrowsFootTerminus for ER diagrams, InhibitorTerminus for
/// biological pathway diagrams).
class TerminusStyle {
public:
    virtual ~TerminusStyle() = default;

    /// Draw the terminus at 'tip', arriving from direction 'from'.
    /// Returns the inset point — where the edge line should actually end
    /// so the line meets the base of the shape, not the tip.
    virtual QPointF draw(QPainter *painter, const QPointF &tip,
                         const QPointF &from, const QPen &edgePen) = 0;

    /// Size contribution for bounding rect / hit testing.
    virtual qreal size() const = 0;
};

/// Filled triangle arrowhead. The classic arrow.
class TriangleTerminus : public TerminusStyle {
public:
    QPointF draw(QPainter *painter, const QPointF &tip,
                 const QPointF &from, const QPen &edgePen) override;
    qreal size() const override;
};

/// Unfilled chevron (V-shape, open back).
class OpenArrowTerminus : public TerminusStyle {
public:
    QPointF draw(QPainter *painter, const QPointF &tip,
                 const QPointF &from, const QPen &edgePen) override;
    qreal size() const override;
};

/// Diamond shape. Configurable filled or unfilled.
class DiamondTerminus : public TerminusStyle {
public:
    explicit DiamondTerminus(bool filled = true) : m_filled(filled) {}
    QPointF draw(QPainter *painter, const QPointF &tip,
                 const QPointF &from, const QPen &edgePen) override;
    qreal size() const override;
private:
    bool m_filled;
};

/// Circle dot. Configurable filled or unfilled.
class CircleTerminus : public TerminusStyle {
public:
    explicit CircleTerminus(bool filled = true) : m_filled(filled) {}
    QPointF draw(QPainter *painter, const QPointF &tip,
                 const QPointF &from, const QPen &edgePen) override;
    qreal size() const override;
private:
    bool m_filled;
};

/// Perpendicular bar across the edge endpoint.
class BarTerminus : public TerminusStyle {
public:
    QPointF draw(QPainter *painter, const QPointF &tip,
                 const QPointF &from, const QPen &edgePen) override;
    qreal size() const override;
};

/// Nothing drawn. Returns tip unchanged.
class NoTerminus : public TerminusStyle {
public:
    QPointF draw(QPainter *painter, const QPointF &tip,
                 const QPointF &from, const QPen &edgePen) override;
    qreal size() const override;
};

} // namespace Graffodil
```

- [ ] **Step 4: Write TerminusStyle.cpp**

```cpp
#include <graffodil/TerminusStyle.h>
#include <QPolygonF>
#include <cmath>

namespace Graffodil {

static constexpr qreal kArrowSize = 10.0;
static constexpr qreal kArrowAngle = M_PI / 6.0; // 30 degrees

// Shared: compute direction angle from 'from' toward 'tip'
static double directionAngle(const QPointF &tip, const QPointF &from)
{
    return std::atan2(-(tip.y() - from.y()), tip.x() - from.x());
}

// Shared: compute inset point (base of arrow along the direction)
static QPointF insetPoint(const QPointF &tip, double angle, double depth)
{
    return tip - QPointF(std::cos(angle) * depth, -std::sin(angle) * depth);
}

// --- TriangleTerminus ---

QPointF TriangleTerminus::draw(QPainter *painter, const QPointF &tip,
                                const QPointF &from, const QPen &edgePen)
{
    const double angle = directionAngle(tip, from);
    const QPointF p1 = tip - QPointF(std::cos(angle - kArrowAngle) * kArrowSize,
                                     -std::sin(angle - kArrowAngle) * kArrowSize);
    const QPointF p2 = tip - QPointF(std::cos(angle + kArrowAngle) * kArrowSize,
                                     -std::sin(angle + kArrowAngle) * kArrowSize);

    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(edgePen.color());
    painter->drawPolygon(QPolygonF{tip, p1, p2});
    painter->restore();

    return insetPoint(tip, angle, kArrowSize * 0.8);
}

qreal TriangleTerminus::size() const { return kArrowSize; }

// --- OpenArrowTerminus ---

QPointF OpenArrowTerminus::draw(QPainter *painter, const QPointF &tip,
                                 const QPointF &from, const QPen &edgePen)
{
    const double angle = directionAngle(tip, from);
    const QPointF p1 = tip - QPointF(std::cos(angle - kArrowAngle) * kArrowSize,
                                     -std::sin(angle - kArrowAngle) * kArrowSize);
    const QPointF p2 = tip - QPointF(std::cos(angle + kArrowAngle) * kArrowSize,
                                     -std::sin(angle + kArrowAngle) * kArrowSize);

    painter->save();
    painter->setPen(QPen(edgePen.color(), edgePen.widthF(), Qt::SolidLine,
                         Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);
    painter->drawPolyline(QPolygonF{p1, tip, p2});
    painter->restore();

    return tip; // Open arrow — line ends at tip
}

qreal OpenArrowTerminus::size() const { return kArrowSize; }

// --- DiamondTerminus ---

QPointF DiamondTerminus::draw(QPainter *painter, const QPointF &tip,
                               const QPointF &from, const QPen &edgePen)
{
    const double angle = directionAngle(tip, from);
    const double halfSize = kArrowSize * 0.6;
    const QPointF mid = insetPoint(tip, angle, halfSize);
    const QPointF back = insetPoint(tip, angle, halfSize * 2);

    const double perpAngle = angle + M_PI / 2.0;
    const QPointF side1 = mid + QPointF(std::cos(perpAngle) * halfSize * 0.5,
                                        -std::sin(perpAngle) * halfSize * 0.5);
    const QPointF side2 = mid - QPointF(std::cos(perpAngle) * halfSize * 0.5,
                                        -std::sin(perpAngle) * halfSize * 0.5);

    painter->save();
    painter->setPen(QPen(edgePen.color(), edgePen.widthF()));
    painter->setBrush(m_filled ? edgePen.color() : Qt::NoBrush);
    painter->drawPolygon(QPolygonF{tip, side1, back, side2});
    painter->restore();

    return back;
}

qreal DiamondTerminus::size() const { return kArrowSize * 1.2; }

// --- CircleTerminus ---

QPointF CircleTerminus::draw(QPainter *painter, const QPointF &tip,
                              const QPointF &from, const QPen &edgePen)
{
    const double angle = directionAngle(tip, from);
    const double radius = kArrowSize * 0.4;
    const QPointF center = insetPoint(tip, angle, radius);

    painter->save();
    painter->setPen(QPen(edgePen.color(), edgePen.widthF()));
    painter->setBrush(m_filled ? edgePen.color() : Qt::NoBrush);
    painter->drawEllipse(center, radius, radius);
    painter->restore();

    return insetPoint(tip, angle, radius * 2);
}

qreal CircleTerminus::size() const { return kArrowSize * 0.8; }

// --- BarTerminus ---

QPointF BarTerminus::draw(QPainter *painter, const QPointF &tip,
                           const QPointF &from, const QPen &edgePen)
{
    const double angle = directionAngle(tip, from);
    const double perpAngle = angle + M_PI / 2.0;
    const double halfLen = kArrowSize * 0.5;

    const QPointF p1 = tip + QPointF(std::cos(perpAngle) * halfLen,
                                     -std::sin(perpAngle) * halfLen);
    const QPointF p2 = tip - QPointF(std::cos(perpAngle) * halfLen,
                                     -std::sin(perpAngle) * halfLen);

    painter->save();
    painter->setPen(QPen(edgePen.color(), edgePen.widthF() + 1, Qt::SolidLine,
                         Qt::FlatCap));
    painter->drawLine(p1, p2);
    painter->restore();

    return tip; // Bar sits at the tip, line ends there
}

qreal BarTerminus::size() const { return kArrowSize * 0.5; }

// --- NoTerminus ---

QPointF NoTerminus::draw(QPainter * /*painter*/, const QPointF &tip,
                          const QPointF & /*from*/, const QPen & /*edgePen*/)
{
    return tip;
}

qreal NoTerminus::size() const { return 0.0; }

} // namespace Graffodil
```

- [ ] **Step 5: Add sources and test to CMake**

Add to `target_sources(graffodil-core ...)`:
```cmake
    src/TerminusStyle.cpp
    include/graffodil/TerminusStyle.h
```

Add to `tests/CMakeLists.txt`:
```cmake
add_executable(tst_terminus tst_terminus.cpp)
target_link_libraries(tst_terminus PRIVATE Graffodil::Core Qt6::Test)
add_test(NAME tst_terminus COMMAND tst_terminus)
```

- [ ] **Step 6: Build and run tests**

```bash
cmake --build build && cd build && ctest -R tst_terminus --output-on-failure
```
Expected: all 4 tests PASS.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat(core): terminus styles — Triangle, OpenArrow, Diamond, Circle, Bar, No"
```

---

### Task 6: GraphEdgeItem

**Files:**
- Create: `src/core/include/graffodil/GraphEdgeItem.h`
- Create: `src/core/src/GraphEdgeItem.cpp`
- Create: `tests/tst_graphedgeitem.cpp`
- Modify: `src/core/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing tests**

`tests/tst_graphedgeitem.cpp`:
```cpp
#include <QTest>
#include <graffodil/GraphEdgeItem.h>
#include <graffodil/EdgePathStrategy.h>
#include <graffodil/TerminusStyle.h>
#include <graffodil/GraphScene.h>
#include "TestHelpers.h"

using namespace Graffodil;
using namespace Graffodil::Test;

class TestGraphEdgeItem : public QObject {
    Q_OBJECT
private slots:
    void edgeImplementsInterface() {
        TestNode src("src", {0, 0}), tgt("tgt", {200, 0});
        auto *strategy = new DirectPathStrategy;
        GraphEdgeItem edge(&src, Side::Right, &tgt, Side::Left, strategy);

        QVERIFY(!edge.edgeId().isEmpty());
        QCOMPARE(edge.sourceNode(), static_cast<IGraphNode *>(&src));
        QCOMPARE(edge.targetNode(), static_cast<IGraphNode *>(&tgt));
        QVERIFY(edge.graphicsItem() != nullptr);
    }

    void adjustUpdatesPath() {
        TestNode src("src", {0, 0}), tgt("tgt", {200, 0});
        auto *strategy = new DirectPathStrategy;
        GraphEdgeItem edge(&src, Side::Right, &tgt, Side::Left, strategy);

        edge.adjust();
        QVERIFY(!edge.path().isEmpty());

        // Move target, readjust
        tgt.setPos(400, 0);
        edge.adjust();
        auto path = edge.path();
        auto last = path.elementAt(path.elementCount() - 1);
        // Last point should reflect new target position
        QVERIFY(last.x > 300.0);
    }

    void setPathStrategyChangesShape() {
        TestNode src("src", {0, 0}), tgt("tgt", {200, 100});
        auto *direct = new DirectPathStrategy;
        GraphEdgeItem edge(&src, Side::Right, &tgt, Side::Left, direct);
        edge.adjust();
        int directElements = edge.path().elementCount();

        auto *bezier = new BezierPathStrategy;
        edge.setPathStrategy(bezier);
        edge.adjust();
        int bezierElements = edge.path().elementCount();

        // Bezier has more elements (cubicTo = 3 control points)
        QVERIFY(bezierElements > directElements);
    }

    void terminusOwnership() {
        TestNode src("src", {0, 0}), tgt("tgt", {200, 0});
        auto *strategy = new DirectPathStrategy;
        GraphEdgeItem edge(&src, Side::Right, &tgt, Side::Left, strategy);

        edge.setTerminus(ArrowEnd::Target, std::make_unique<TriangleTerminus>());
        edge.setTerminus(ArrowEnd::Source, std::make_unique<NoTerminus>());
        // No crash — ownership transferred correctly
        edge.adjust();
        QVERIFY(!edge.path().isEmpty());
    }

    void edgeInScene() {
        GraphScene scene;
        auto *src = new TestNode("src", {0, 0});
        auto *tgt = new TestNode("tgt", {200, 0});
        scene.addNode(src);
        scene.addNode(tgt);

        auto *strategy = new DirectPathStrategy;
        auto *edge = new GraphEdgeItem(src, Side::Right, tgt, Side::Left, strategy);
        scene.addEdge(edge);

        QCOMPARE(scene.edges().size(), 1);
        QCOMPARE(scene.edgeBetween("src", "tgt"), static_cast<IGraphEdge *>(edge));
    }
};

QTEST_MAIN(TestGraphEdgeItem)
#include "tst_graphedgeitem.moc"
```

- [ ] **Step 2: Run tests — verify compilation fails**

- [ ] **Step 3: Write GraphEdgeItem.h**

```cpp
#pragma once

#include <graffodil/IGraphEdge.h>
#include <graffodil/Types.h>
#include <QGraphicsPathItem>
#include <memory>

namespace Graffodil {

class IGraphNode;
class EdgePathStrategy;
class TerminusStyle;

/// Provided edge implementation. Composes an EdgePathStrategy for path
/// shape and TerminusStyle for endpoint rendering. Consumers who don't
/// need custom edge rendering use this directly.
///
/// Integration: call adjust() when source/target nodes move.
/// The edge recomputes its path from current anchor positions.
class GraphEdgeItem : public QGraphicsPathItem, public IGraphEdge {
public:
    GraphEdgeItem(IGraphNode *source, Side sourceSide,
                  IGraphNode *target, Side targetSide,
                  EdgePathStrategy *strategy);
    ~GraphEdgeItem() override;

    // --- IGraphEdge ---
    QString edgeId() const override;
    IGraphNode *sourceNode() const override;
    IGraphNode *targetNode() const override;
    QGraphicsItem *graphicsItem() override;

    // --- Configuration ---

    /// Replace the path computation strategy. Takes ownership.
    void setPathStrategy(EdgePathStrategy *strategy);

    /// Set terminus style for an endpoint. Takes ownership.
    void setTerminus(ArrowEnd end, std::unique_ptr<TerminusStyle> style);

    /// Line style (color, dash pattern, width).
    void setPen(const QPen &pen);

    void setHighlighted(bool highlighted);

    /// Provide waypoints for path strategies that use them (Orthogonal, Polyline).
    void setWaypoints(const QList<QPointF> &waypoints);

    // --- Geometry update ---

    /// Recomputes path from current node anchor positions through
    /// the active strategy. Call when nodes move.
    void adjust();

    Side sourceSide() const { return m_sourceSide; }
    Side targetSide() const { return m_targetSide; }

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

private:
    QString m_id;
    IGraphNode *m_source;
    IGraphNode *m_target;
    Side m_sourceSide;
    Side m_targetSide;
    std::unique_ptr<EdgePathStrategy> m_strategy;
    std::unique_ptr<TerminusStyle> m_sourceTerminus;
    std::unique_ptr<TerminusStyle> m_targetTerminus;
    QList<QPointF> m_waypoints;
    bool m_highlighted = false;
};

} // namespace Graffodil
```

- [ ] **Step 4: Write GraphEdgeItem.cpp**

```cpp
#include <graffodil/GraphEdgeItem.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/EdgePathStrategy.h>
#include <graffodil/TerminusStyle.h>
#include <QPainter>
#include <QUuid>

namespace Graffodil {

GraphEdgeItem::GraphEdgeItem(IGraphNode *source, Side sourceSide,
                             IGraphNode *target, Side targetSide,
                             EdgePathStrategy *strategy)
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_source(source)
    , m_target(target)
    , m_sourceSide(sourceSide)
    , m_targetSide(targetSide)
    , m_strategy(strategy)
{
    setFlag(ItemIsSelectable);
}

GraphEdgeItem::~GraphEdgeItem() = default;

QString GraphEdgeItem::edgeId() const { return m_id; }
IGraphNode *GraphEdgeItem::sourceNode() const { return m_source; }
IGraphNode *GraphEdgeItem::targetNode() const { return m_target; }
QGraphicsItem *GraphEdgeItem::graphicsItem() { return this; }

void GraphEdgeItem::setPathStrategy(EdgePathStrategy *strategy)
{
    m_strategy.reset(strategy);
}

void GraphEdgeItem::setTerminus(ArrowEnd end, std::unique_ptr<TerminusStyle> style)
{
    if (end == ArrowEnd::Source)
        m_sourceTerminus = std::move(style);
    else
        m_targetTerminus = std::move(style);
}

void GraphEdgeItem::setPen(const QPen &pen)
{
    QGraphicsPathItem::setPen(pen);
}

void GraphEdgeItem::setHighlighted(bool highlighted)
{
    m_highlighted = highlighted;
    update();
}

void GraphEdgeItem::setWaypoints(const QList<QPointF> &waypoints)
{
    m_waypoints = waypoints;
}

void GraphEdgeItem::adjust()
{
    if (!m_strategy || !m_source || !m_target)
        return;

    prepareGeometryChange();

    const QPointF srcAnchor = m_source->anchorPoint(m_sourceSide);
    const QPointF tgtAnchor = m_target->anchorPoint(m_targetSide);

    QPainterPath path = m_strategy->computePath(
        srcAnchor, m_sourceSide, tgtAnchor, m_targetSide, m_waypoints);

    setPath(path);
}

void GraphEdgeItem::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (path().isEmpty())
        return;

    painter->setRenderHint(QPainter::Antialiasing);

    QPen edgePen = pen();
    if (m_highlighted) {
        edgePen.setWidthF(edgePen.widthF() + 1.0);
    }
    if (isSelected()) {
        edgePen.setColor(edgePen.color().lighter(150));
    }

    painter->setPen(edgePen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path());

    // Draw terminus styles at endpoints
    const QPointF srcAnchor = m_source->anchorPoint(m_sourceSide);
    const QPointF tgtAnchor = m_target->anchorPoint(m_targetSide);

    if (m_sourceTerminus) {
        // Source terminus: tip is at source, "arriving from" is the next point
        QPointF fromDir = tgtAnchor; // approximate direction
        m_sourceTerminus->draw(painter, srcAnchor, fromDir, edgePen);
    }

    if (m_targetTerminus) {
        // Target terminus: tip is at target, arriving from source direction
        m_targetTerminus->draw(painter, tgtAnchor, srcAnchor, edgePen);
    }
}

} // namespace Graffodil
```

- [ ] **Step 5: Add sources and test to CMake**

Add to `target_sources(graffodil-core ...)`:
```cmake
    src/GraphEdgeItem.cpp
    include/graffodil/GraphEdgeItem.h
```

Add to `tests/CMakeLists.txt`:
```cmake
add_executable(tst_graphedgeitem tst_graphedgeitem.cpp)
target_link_libraries(tst_graphedgeitem PRIVATE Graffodil::Core Qt6::Test)
target_include_directories(tst_graphedgeitem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_graphedgeitem COMMAND tst_graphedgeitem)
```

- [ ] **Step 6: Build and run tests**

```bash
cmake --build build && cd build && ctest -R tst_graphedgeitem --output-on-failure
```
Expected: all 5 tests PASS.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat(core): GraphEdgeItem — composable edge with path strategies and terminus styles"
```

---

### Task 7: Tool Framework + SelectMoveTool

**Files:**
- Create: `src/core/include/graffodil/GraphTool.h`
- Create: `src/core/src/GraphTool.cpp`
- Create: `src/core/include/graffodil/SelectMoveTool.h`
- Create: `src/core/src/SelectMoveTool.cpp`
- Create: `tests/tst_tools.cpp`
- Modify: `src/core/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `src/core/src/GraphScene.cpp` (wire tool dispatch)

- [ ] **Step 1: Write the failing tests**

`tests/tst_tools.cpp`:
```cpp
#include <QTest>
#include <QSignalSpy>
#include <QGraphicsSceneMouseEvent>
#include <graffodil/GraphScene.h>
#include <graffodil/GraphTool.h>
#include <graffodil/SelectMoveTool.h>
#include "TestHelpers.h"

using namespace Graffodil;
using namespace Graffodil::Test;

class TestTools : public QObject {
    Q_OBJECT
private slots:
    void toolActivateDeactivate() {
        GraphScene scene;
        SelectMoveTool tool;

        scene.setActiveTool(&tool);
        QCOMPARE(scene.activeTool(), &tool);
        QCOMPARE(tool.scene(), &scene);
    }

    void selectToolClickSelectsNode() {
        GraphScene scene;
        SelectMoveTool tool;
        scene.setActiveTool(&tool);

        auto *n1 = new TestNode("n1", {0, 0});
        n1->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
        scene.addNode(n1);

        // Simulate click on node center
        QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
        press.setScenePos(QPointF(50, 30));  // center of 100x60 node at (0,0)
        press.setButton(Qt::LeftButton);
        scene.mousePressEvent(&press);

        QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
        release.setScenePos(QPointF(50, 30));
        release.setButton(Qt::LeftButton);
        scene.mouseReleaseEvent(&release);

        QCOMPARE(scene.selectedNodes().size(), 1);
    }

    void selectToolStatusMessage() {
        SelectMoveTool tool;
        QSignalSpy spy(&tool, &GraphTool::statusMessage);
        GraphScene scene;
        scene.setActiveTool(&tool);
        // Activation should emit status message
        QVERIFY(spy.count() >= 1);
    }
};

QTEST_MAIN(TestTools)
#include "tst_tools.moc"
```

- [ ] **Step 2: Run tests — verify compilation fails**

- [ ] **Step 3: Write GraphTool.h and GraphTool.cpp**

`src/core/include/graffodil/GraphTool.h`:
```cpp
#pragma once

#include <QObject>

class QGraphicsSceneMouseEvent;
class QKeyEvent;

namespace Graffodil {

class GraphScene;

/// Base class for interaction tools. Tools receive scene events,
/// emit signals for consumer response, and never mutate the data
/// model directly.
///
/// Consumer integration: wire tool activation to QAction or KActionCollection.
/// The tools themselves are pure Qt — no KDE dependency.
///
/// Example:
///   auto selectAction = actionCollection()->addAction("graph_select");
///   connect(selectAction, &QAction::triggered, this, [this]() {
///       m_scene->setActiveTool(m_selectTool);
///   });
class GraphTool : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

    /// Called by GraphScene when this tool becomes active.
    /// Override to set cursors, create preview items, etc.
    virtual void activate(GraphScene *scene);

    /// Called by GraphScene when another tool is activated.
    /// Override to remove preview items, restore cursors.
    virtual void deactivate();

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) = 0;
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

    GraphScene *scene() const;

signals:
    /// Status hints for toolbar/statusbar display.
    /// Consumer connects this to their status widget.
    void statusMessage(const QString &msg);

protected:
    GraphScene *m_scene = nullptr;
};

} // namespace Graffodil
```

`src/core/src/GraphTool.cpp`:
```cpp
#include <graffodil/GraphTool.h>
#include <graffodil/GraphScene.h>

namespace Graffodil {

void GraphTool::activate(GraphScene *scene)
{
    m_scene = scene;
}

void GraphTool::deactivate()
{
    m_scene = nullptr;
}

void GraphTool::keyPressEvent(QKeyEvent *) {}
void GraphTool::keyReleaseEvent(QKeyEvent *) {}

GraphScene *GraphTool::scene() const
{
    return m_scene;
}

} // namespace Graffodil
```

- [ ] **Step 4: Write SelectMoveTool.h and SelectMoveTool.cpp**

`src/core/include/graffodil/SelectMoveTool.h`:
```cpp
#pragma once

#include <graffodil/GraphTool.h>
#include <graffodil/Types.h>
#include <QList>
#include <QPointF>

namespace Graffodil {

class IGraphNode;
class IGraphEdge;

/// Select and move tool. Transplanted from PlanStan SelectMoveTool
/// and Corbomite CanvasTool (SelectMove mode).
///
/// - Click: select node or edge
/// - Drag on node: move
/// - Rubber-band drag on empty space: multi-select
/// - Shift+click: toggle selection
/// - Delete key: emits deleteRequested (consumer handles deletion and undo)
class SelectMoveTool : public GraphTool {
    Q_OBJECT
public:
    using GraphTool::GraphTool;

    void activate(GraphScene *scene) override;
    void deactivate() override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

signals:
    /// Consumer handles deletion and undo. Tool doesn't mutate the scene.
    void deleteRequested(const QList<IGraphNode *> &nodes,
                         const QList<IGraphEdge *> &edges);

    /// Consumer pushes undo commands for position changes.
    void nodeMoved(IGraphNode *node, const QPointF &oldPos, const QPointF &newPos);

private:
    IGraphNode *m_dragNode = nullptr;
    QPointF m_dragStartPos;
    bool m_dragging = false;
};

} // namespace Graffodil
```

`src/core/src/SelectMoveTool.cpp`:
```cpp
#include <graffodil/SelectMoveTool.h>
#include <graffodil/GraphScene.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>

namespace Graffodil {

void SelectMoveTool::activate(GraphScene *scene)
{
    GraphTool::activate(scene);
    emit statusMessage(tr("Click to select, drag to move"));
}

void SelectMoveTool::deactivate()
{
    m_dragNode = nullptr;
    m_dragging = false;
    GraphTool::deactivate();
}

void SelectMoveTool::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_scene)
        return;

    // Check if we hit a node
    auto *item = m_scene->itemAt(event->scenePos(), QTransform());
    if (!item)
        return;

    // Find the IGraphNode that owns this item
    for (auto *node : m_scene->nodes()) {
        if (node->graphicsItem() == item || node->graphicsItem()->isAncestorOf(item)) {
            m_dragNode = node;
            m_dragStartPos = node->graphicsItem()->pos();
            m_dragging = false;

            if (!(event->modifiers() & Qt::ShiftModifier)) {
                // Clear other selections unless shift-clicking
                m_scene->clearSelection();
            }
            node->graphicsItem()->setSelected(true);
            return;
        }
    }

    // Check edges
    for (auto *edge : m_scene->edges()) {
        if (edge->graphicsItem() == item || edge->graphicsItem()->isAncestorOf(item)) {
            if (!(event->modifiers() & Qt::ShiftModifier))
                m_scene->clearSelection();
            edge->graphicsItem()->setSelected(true);
            return;
        }
    }

    // Empty space — clear selection
    if (!(event->modifiers() & Qt::ShiftModifier))
        m_scene->clearSelection();
}

void SelectMoveTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_dragNode || !m_scene)
        return;

    m_dragging = true;
    QPointF delta = event->scenePos() - event->lastScenePos();
    QPointF newPos = m_dragNode->graphicsItem()->pos() + delta;
    m_dragNode->graphicsItem()->setPos(newPos);
}

void SelectMoveTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    if (m_dragNode && m_dragging) {
        QPointF newPos = m_dragNode->graphicsItem()->pos();
        if (m_dragStartPos != newPos)
            emit nodeMoved(m_dragNode, m_dragStartPos, newPos);
    }
    m_dragNode = nullptr;
    m_dragging = false;
}

void SelectMoveTool::keyPressEvent(QKeyEvent *event)
{
    if (!m_scene)
        return;

    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        auto nodes = m_scene->selectedNodes();
        auto edges = m_scene->selectedEdges();
        if (!nodes.isEmpty() || !edges.isEmpty())
            emit deleteRequested(nodes, edges);
    }
}

} // namespace Graffodil
```

- [ ] **Step 5: Wire tool dispatch in GraphScene.cpp**

Update the event handlers in `GraphScene.cpp`:

```cpp
void GraphScene::setActiveTool(GraphTool *tool)
{
    if (m_activeTool)
        m_activeTool->deactivate();
    m_activeTool = tool;
    if (m_activeTool)
        m_activeTool->activate(this);
}

void GraphScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    if (m_activeTool)
        m_activeTool->mousePressEvent(e);
    else
        QGraphicsScene::mousePressEvent(e);
}

void GraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    if (m_activeTool)
        m_activeTool->mouseMoveEvent(e);
    else
        QGraphicsScene::mouseMoveEvent(e);
}

void GraphScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    if (m_activeTool)
        m_activeTool->mouseReleaseEvent(e);
    else
        QGraphicsScene::mouseReleaseEvent(e);
}

void GraphScene::keyPressEvent(QKeyEvent *e)
{
    if (m_activeTool)
        m_activeTool->keyPressEvent(e);
    else
        QGraphicsScene::keyPressEvent(e);
}
```

Also add `#include <graffodil/GraphTool.h>` to GraphScene.cpp.

- [ ] **Step 6: Add sources and test to CMake**

Add to `target_sources(graffodil-core ...)`:
```cmake
    src/GraphTool.cpp
    src/SelectMoveTool.cpp
    include/graffodil/GraphTool.h
    include/graffodil/SelectMoveTool.h
```

Add to `tests/CMakeLists.txt`:
```cmake
add_executable(tst_tools tst_tools.cpp)
target_link_libraries(tst_tools PRIVATE Graffodil::Core Qt6::Test)
target_include_directories(tst_tools PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_tools COMMAND tst_tools)
```

- [ ] **Step 7: Build and run tests**

```bash
cmake --build build && cd build && ctest --output-on-failure
```
Expected: all tests PASS (tst_graphscene, tst_edgepathstrategies, tst_terminus, tst_graphedgeitem, tst_tools).

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat(core): tool framework + SelectMoveTool"
```

---

### Task 8: CreateEdgeTool + PanZoomTool + AnchorHighlight

**Files:**
- Create: `src/core/include/graffodil/CreateEdgeTool.h`
- Create: `src/core/src/CreateEdgeTool.cpp`
- Create: `src/core/include/graffodil/PanZoomTool.h`
- Create: `src/core/src/PanZoomTool.cpp`
- Create: `src/core/include/graffodil/AnchorHighlight.h`
- Create: `src/core/src/AnchorHighlight.cpp`
- Modify: `tests/tst_tools.cpp` (add CreateEdgeTool and PanZoomTool tests)
- Modify: `src/core/CMakeLists.txt`

This task follows the same TDD pattern as Task 7. The implementations are transplanted from PlanStan's `createedgetool.cpp` and `panzoomtool.cpp`, adapted to use `IGraphNode` and emit `edgeRequested` instead of directly creating edges.

**Key adaptations from source:**
- `CreateEdgeTool`: Replace `GraphNodeItem*` with `IGraphNode*`. Replace direct edge creation with `emit edgeRequested(source, sourceSide, target, targetSide)`. Use `AnchorHighlight` for visual feedback during drag.
- `PanZoomTool`: Replace `QGraphicsView*` access with `scene()->views().first()`. Keep zoom-to-cursor and pan logic.
- `AnchorHighlight`: Draw 4 small circles at a node's anchor points. Highlight the nearest one to the cursor. Added/removed from scene by CreateEdgeTool.

**Tests to add to tst_tools.cpp:**
- `createEdgeToolEmitsRequest`: Verify signal emission with correct source/target/sides
- `createEdgeToolPreventsSelfLoop`: Click and release on same node — no signal
- `panZoomToolActivates`: Verify tool can be set as active

- [ ] **Step 1-7: Follow same TDD pattern as Task 7**

Write tests, verify fail, implement headers, implement .cpp, add to CMake, verify pass, commit.

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat(core): CreateEdgeTool, PanZoomTool, AnchorHighlight"
```

---

### Task 9: Utility Widgets — GraphMinimap + ConnectorItem

**Files:**
- Create: `src/core/include/graffodil/GraphMinimap.h`
- Create: `src/core/src/GraphMinimap.cpp`
- Create: `src/core/include/graffodil/ConnectorItem.h`
- Create: `src/core/src/ConnectorItem.cpp`
- Modify: `src/core/CMakeLists.txt`

Transplant from PlanStan's `graphminimap.cpp` (147 LOC) and `connectoritem.cpp` (36 LOC). Adapt minimap to work with generic `GraphScene` instead of `DependencyScene`.

ConnectorItem is trivial — a small filled ellipse at a waypoint position.

- [ ] **Step 1-6: Implement and add to CMake**

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat(core): GraphMinimap and ConnectorItem utility widgets"
```

---

### Task 10: Circular Layout Module

**Files:**
- Create: `src/circular/include/graffodil/CircularLayout.h`
- Create: `src/circular/src/CircularLayout.cpp`
- Create: `tests/tst_circular.cpp`
- Modify: `src/circular/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing tests**

`tests/tst_circular.cpp`:
```cpp
#include <QTest>
#include <graffodil/CircularLayout.h>
#include <graffodil/GraphScene.h>
#include "TestHelpers.h"
#include <cmath>

using namespace Graffodil;
using namespace Graffodil::Test;

class TestCircularLayout : public QObject {
    Q_OBJECT
private slots:
    void threeNodesFormTriangle() {
        TestNode n1("n1"), n2("n2"), n3("n3");
        CircularLayout layout;
        layout.setRadius(100);
        layout.setNodes({&n1, &n2, &n3});
        layout.setEdges({});

        auto result = layout.compute();
        QCOMPARE(result.nodePositions.size(), 3);

        // All nodes should be ~100px from origin
        for (const auto &pos : result.nodePositions) {
            double dist = std::sqrt(pos.x() * pos.x() + pos.y() * pos.y());
            QVERIFY(std::abs(dist - 100.0) < 1.0);
        }
    }

    void centerNodeAtOrigin() {
        TestNode n1("center"), n2("a"), n3("b"), n4("c");
        CircularLayout layout;
        layout.setRadius(100);
        layout.setCenterNode("center");
        layout.setNodes({&n1, &n2, &n3, &n4});
        layout.setEdges({});

        auto result = layout.compute();
        QCOMPARE(result.nodePositions["center"], QPointF(0, 0));

        // Other 3 nodes on the circle
        for (const auto &id : {"a", "b", "c"}) {
            auto pos = result.nodePositions[id];
            double dist = std::sqrt(pos.x() * pos.x() + pos.y() * pos.y());
            QVERIFY(std::abs(dist - 100.0) < 1.0);
        }
    }

    void singleNodeAtOrigin() {
        TestNode n1("solo");
        CircularLayout layout;
        layout.setRadius(100);
        layout.setNodes({&n1});
        layout.setEdges({});

        auto result = layout.compute();
        QCOMPARE(result.nodePositions["solo"], QPointF(0, 0));
    }

    void isNotIterative() {
        CircularLayout layout;
        QVERIFY(!layout.isIterative());
    }
};

QTEST_MAIN(TestCircularLayout)
#include "tst_circular.moc"
```

- [ ] **Step 2: Run tests — verify compilation fails**

- [ ] **Step 3: Write CircularLayout.h**

```cpp
#pragma once

#include <graffodil/LayoutEngine.h>

namespace Graffodil {

/// Simple circular layout for small graphs (3-20 nodes).
/// Designed for the sync topology use case.
class CircularLayout : public LayoutEngine {
public:
    void setNodes(const QList<IGraphNode *> &nodes) override;
    void setEdges(const QList<IGraphEdge *> &edges) override;

    LayoutResult compute() override;
    bool isIterative() const override { return false; }

    void setRadius(qreal r);

    /// Optional: place one node at the center, rest on the circle.
    void setCenterNode(const QString &id);

private:
    QList<IGraphNode *> m_nodes;
    QList<IGraphEdge *> m_edges;
    qreal m_radius = 100.0;
    QString m_centerNodeId;
};

} // namespace Graffodil
```

- [ ] **Step 4: Write CircularLayout.cpp**

```cpp
#include <graffodil/CircularLayout.h>
#include <graffodil/IGraphNode.h>
#include <cmath>

namespace Graffodil {

void CircularLayout::setNodes(const QList<IGraphNode *> &nodes) { m_nodes = nodes; }
void CircularLayout::setEdges(const QList<IGraphEdge *> &edges) { m_edges = edges; }
void CircularLayout::setRadius(qreal r) { m_radius = r; }
void CircularLayout::setCenterNode(const QString &id) { m_centerNodeId = id; }

LayoutResult CircularLayout::compute()
{
    LayoutResult result;

    if (m_nodes.isEmpty())
        return result;

    // Single node goes at origin
    if (m_nodes.size() == 1) {
        result.nodePositions[m_nodes.first()->nodeId()] = {0, 0};
        return result;
    }

    // Separate center node from circle nodes
    QList<IGraphNode *> circleNodes;
    for (auto *node : m_nodes) {
        if (node->nodeId() == m_centerNodeId) {
            result.nodePositions[node->nodeId()] = {0, 0};
        } else {
            circleNodes.append(node);
        }
    }

    // If no center node was set, all nodes go on circle
    if (m_centerNodeId.isEmpty())
        circleNodes = m_nodes;

    const int n = circleNodes.size();
    if (n == 0)
        return result;

    const double angleStep = 2.0 * M_PI / n;
    for (int i = 0; i < n; ++i) {
        const double angle = angleStep * i - M_PI / 2.0; // start at top
        const double x = m_radius * std::cos(angle);
        const double y = m_radius * std::sin(angle);
        result.nodePositions[circleNodes[i]->nodeId()] = {x, y};
    }

    return result;
}

} // namespace Graffodil
```

- [ ] **Step 5: Update Circular CMakeLists.txt**

Replace placeholder with real sources:
```cmake
target_sources(graffodil-circular PRIVATE
    src/CircularLayout.cpp
    include/graffodil/CircularLayout.h
)
```

Remove `src/placeholder.cpp` and its target_sources line.

- [ ] **Step 6: Add test to tests/CMakeLists.txt**

```cmake
add_executable(tst_circular tst_circular.cpp)
target_link_libraries(tst_circular PRIVATE Graffodil::Circular Qt6::Test)
target_include_directories(tst_circular PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_circular COMMAND tst_circular)
```

- [ ] **Step 7: Build and run tests**

```bash
cmake --build build && cd build && ctest -R tst_circular --output-on-failure
```
Expected: all 4 tests PASS.

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat(circular): CircularLayout — simple radial layout for small graphs"
```

---

### Task 11: Sugiyama Layout Module

**Files:**
- Create: `src/sugiyama/include/graffodil/SugiyamaLayout.h`
- Create: `src/sugiyama/src/SugiyamaLayout.cpp`
- Create: `tests/tst_sugiyama.cpp`
- Modify: `src/sugiyama/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Transplant from:** `~/dev/PlanStan/libs/graph/src/graphlayout.cpp` (800 LOC) and `~/dev/PlanStan/libs/graph/include/graphlayout.h` (49 LOC).

**Adaptations needed:**
1. Replace `DependencyGraph&` input with `QList<IGraphNode*>` + `QList<IGraphEdge*>` — build internal adjacency from `edge->sourceNode()->nodeId()` / `edge->targetNode()->nodeId()`
2. Replace `graph.topologicalOrder()` with internal topological sort (the layout already computes its own layers from BFS)
3. Replace `graph.criticalPath()` with `setSpine(QStringList)` — the caller provides the spine node IDs
4. Rename namespace from `GraphLayout` to `Graffodil::SugiyamaLayout`
5. Remove references to `IncidenceRef`, `PlanningMetadataStore`, and WBS index ordering — replace with alphabetical ID ordering for deterministic component ordering

**Tests to write** (transplant relevant portions from `~/dev/PlanStan/tests/planning/tst_dependencygraph.cpp` plus new interface tests):
- `linearChainProducesLayers`: A→B→C produces 3 layers, left-to-right
- `nodesDoNotOverlap`: verify spacing between all node pairs
- `spineAlignmentCenters`: with spine set, spine nodes share similar Y coordinate
- `disconnectedComponentsHandled`: two separate subgraphs both get positioned
- `singleNodeWorks`: one node, no edges

- [ ] **Step 1-7: TDD — write tests, transplant implementation, verify pass**

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat(sugiyama): SugiyamaLayout — 5-phase layered layout with spine alignment"
```

---

### Task 12: Force Layout Module (depends on Task 13)

**Files:**
- Create: `src/force/include/graffodil/ForceLayout.h`
- Create: `src/force/src/ForceLayout.cpp`
- Create: `src/force/include/graffodil/MultilevelLayout.h`
- Create: `src/force/src/MultilevelLayout.cpp`
- Create: `tests/tst_forcelayout.cpp`
- Modify: `src/force/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Transplant from:** `~/dev/Corbomite/libs/forcegraph/src/ForceLayoutEngine.cpp` (~600 LOC) and `~/dev/Corbomite/libs/forcegraph/src/MultilevelLayout.cpp` (~250 LOC).

**Adaptations needed:**
1. Replace `GraphNode`/`GraphEdge` structs with `IGraphNode*`/`IGraphEdge*` — build internal arrays indexed by integer
2. Move QuadTree dependency to internal use (QuadTree implementation also goes into this module initially; will move to Batch in Task 14 if needed)
3. Implement `LayoutEngine` interface: `compute()` runs multilevel + simulation to convergence, `tick()` runs one step, `currentPositions()` returns latest positions
4. `pinNode()`/`unpinNode()` mark nodes as fixed during simulation

**Tests** (transplant from `~/dev/Corbomite/libs/forcegraph/tests/tst_forcelayout.cpp` plus new):
- `convergesOnSmallGraph`: 10 nodes, verify simulation stops
- `connectedNodesCloserThanDisconnected`: after layout, linked pairs are nearer than unlinked
- `pinnedNodeDoesNotMove`: pin a node, run simulation, verify position unchanged
- `tickReturnsfalseWhenConverged`: iterate until tick() returns false
- `isIterativeTrue`: verify `isIterative()` returns true

Note: The Force module needs a QuadTree internally. **Do Task 13 (SpatialGrid + QuadTree) before this task** so the QuadTree is available as a public header in `Graffodil::Batch`. Then add `Graffodil::Batch` as a dependency of `Graffodil::Force` in `src/force/CMakeLists.txt`: `target_link_libraries(graffodil-force PUBLIC Graffodil::Core PRIVATE Graffodil::Batch)`.

- [ ] **Step 1-7: TDD — write tests, transplant implementation, verify pass**

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat(force): ForceLayout + MultilevelLayout — ForceAtlas2 with Barnes-Hut"
```

---

### Task 13: Batch Rendering Module — SpatialGrid + QuadTree (public)

**Files:**
- Create: `src/batch/include/graffodil/SpatialGrid.h`
- Create: `src/batch/include/graffodil/QuadTree.h`
- Create: `src/batch/src/QuadTree.cpp`
- Create: `tests/tst_spatialgrid.cpp`
- Create: `tests/tst_quadtree.cpp`
- Modify: `src/batch/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Transplant from:**
- `~/dev/Corbomite/libs/forcegraph/include/forcegraph/SpatialGrid.h` (97 LOC, header-only)
- `~/dev/Corbomite/libs/forcegraph/src/QuadTree.cpp` + header

**Adaptations:** Rename namespace to `Graffodil`. No other changes — these are pure algorithms with zero domain coupling.

**Tests** (transplant from `~/dev/Corbomite/libs/forcegraph/tests/tst_quadtree.cpp` plus new SpatialGrid tests):
- **SpatialGrid:** `queryReturnsVisibleIndices`, `emptyGridReturnsEmpty`, `rebuildWithNewPositions`
- **QuadTree:** `repulsionPushesApart`, `farNodesApproximated` (theta test), `singleNode`

- [ ] **Step 1-7: TDD — write tests, transplant, verify pass**

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat(batch): SpatialGrid and QuadTree — spatial acceleration structures"
```

---

### Task 14: Batch Rendering Module — BatchRenderer

**Files:**
- Create: `src/batch/include/graffodil/BatchRenderer.h`
- Create: `src/batch/src/BatchRenderer.cpp`
- Create: `src/batch/src/BatchNodeItem.h`
- Create: `src/batch/src/BatchNodeItem.cpp`
- Create: `src/batch/src/BatchEdgeItem.h`
- Create: `src/batch/src/BatchEdgeItem.cpp`
- Create: `tests/tst_batchrenderer.cpp`
- Modify: `src/batch/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Transplant from:** `~/dev/Corbomite/libs/forcegraph/src/BatchNodeItem.cpp` and `BatchEdgeItem.cpp`.

**Adaptations:**
1. Replace `ForceGraphNode`/`ForceGraphEdge` with generic data pulled from `GraphScene` nodes/edges
2. `BatchRenderer::synchronize()` reads `scene->nodes()` positions and rebuilds batch data
3. `setBatchMode(true)` hides individual items, shows batch items. `setBatchMode(false)` reverses.
4. BatchNodeItem/BatchEdgeItem are internal (headers in `src/`, not `include/`)

**Tests:**
- `batchModeHidesIndividualItems`: add nodes, enable batch, verify individual items hidden
- `synchronizeUpdatesPositions`: move nodes, synchronize, verify batch data reflects new positions
- `autoThresholdSwitches`: set threshold, add nodes above it, verify batch mode enabled

- [ ] **Step 1-7: TDD — write tests, transplant, verify pass**

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat(batch): BatchRenderer — single-paint-call rendering for 10K+ nodes"
```

---

### Task 15: Integration Test + Final Verification

**Files:**
- Create: `tests/tst_integration.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write integration test**

`tests/tst_integration.cpp`:
```cpp
#include <QTest>
#include <graffodil/GraphScene.h>
#include <graffodil/GraphEdgeItem.h>
#include <graffodil/EdgePathStrategy.h>
#include <graffodil/TerminusStyle.h>
#include <graffodil/CircularLayout.h>
#include <graffodil/SelectMoveTool.h>
#include <graffodil/CreateEdgeTool.h>
#include <graffodil/PanZoomTool.h>
#include "TestHelpers.h"
#include <cmath>

using namespace Graffodil;
using namespace Graffodil::Test;

class TestIntegration : public QObject {
    Q_OBJECT
private slots:
    void fullWorkflow() {
        // Build a 5-node graph
        GraphScene scene;
        QList<TestNode *> nodes;
        for (int i = 0; i < 5; ++i) {
            auto *n = new TestNode(QString("n%1").arg(i));
            n->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
            scene.addNode(n);
            nodes.append(n);
        }
        QCOMPARE(scene.nodes().size(), 5);

        // Add edges with bezier strategy and triangle terminus
        for (int i = 0; i < 4; ++i) {
            auto *edge = new GraphEdgeItem(
                nodes[i], Side::Right, nodes[i + 1], Side::Left,
                new BezierPathStrategy);
            edge->setTerminus(ArrowEnd::Target, std::make_unique<TriangleTerminus>());
            scene.addEdge(edge);
        }
        QCOMPARE(scene.edges().size(), 4);

        // Apply circular layout
        CircularLayout layout;
        layout.setRadius(200);
        layout.setNodes(scene.nodes());
        layout.setEdges(scene.edges());
        auto result = layout.compute();
        scene.applyLayout(result);

        // Verify no nodes at origin (all on circle)
        for (auto *node : scene.nodes()) {
            QPointF pos = node->graphicsItem()->pos();
            double dist = std::sqrt(pos.x() * pos.x() + pos.y() * pos.y());
            QVERIFY2(dist > 50.0,
                qPrintable(QString("Node %1 too close to origin: %2")
                    .arg(node->nodeId()).arg(dist)));
        }

        // Adjust edges after layout
        for (auto *edge : scene.edges()) {
            auto *edgeItem = dynamic_cast<GraphEdgeItem *>(edge);
            if (edgeItem)
                edgeItem->adjust();
        }

        // Verify edges have non-empty paths
        for (auto *edge : scene.edges()) {
            auto *edgeItem = dynamic_cast<GraphEdgeItem *>(edge);
            if (edgeItem)
                QVERIFY(!edgeItem->path().isEmpty());
        }

        // Activate select tool
        SelectMoveTool selectTool;
        scene.setActiveTool(&selectTool);
        QCOMPARE(scene.activeTool(), &selectTool);

        // Switch to create edge tool
        CreateEdgeTool edgeTool;
        scene.setActiveTool(&edgeTool);
        QCOMPARE(scene.activeTool(), &edgeTool);

        // Switch to pan/zoom
        PanZoomTool panTool;
        scene.setActiveTool(&panTool);
        QCOMPARE(scene.activeTool(), &panTool);
    }
};

QTEST_MAIN(TestIntegration)
#include "tst_integration.moc"
```

- [ ] **Step 2: Add to tests/CMakeLists.txt**

```cmake
add_executable(tst_integration tst_integration.cpp)
target_link_libraries(tst_integration PRIVATE
    Graffodil::Core Graffodil::Circular Qt6::Test)
target_include_directories(tst_integration PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_integration COMMAND tst_integration)
```

- [ ] **Step 3: Build and run ALL tests**

```bash
cmake --build build && cd build && ctest --output-on-failure
```
Expected: ALL tests PASS across all modules.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "test: integration test — full workflow with scene, edges, layout, tools"
```

---

### Task 16: Clean Up + CLAUDE.md

**Files:**
- Create: `CLAUDE.md`
- Remove: all `placeholder.cpp` files (should already be gone from individual tasks)
- Verify: `compile_commands.json` symlink exists

- [ ] **Step 1: Create CLAUDE.md**

```markdown
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
- `Graffodil::Force` — ForceAtlas2 + multilevel (depends: Core)
- `Graffodil::Circular` — Circular layout (depends: Core)
- `Graffodil::Batch` — Batch rendering for 10K+ nodes (depends: Core)
```

- [ ] **Step 2: Remove any remaining placeholder files**

```bash
find . -name "placeholder.cpp" -delete
```

Verify no target_sources references to placeholder remain.

- [ ] **Step 3: Final build and test**

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build && cd build && ctest --output-on-failure
```
Expected: clean build, all tests pass.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "chore: CLAUDE.md, clean up placeholders, final verification"
```
