# Edge Contract Implementation Plan (Phase 2)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Close the two contract holes in the edge abstraction (evaluation findings A1, A2) and unify strategy ownership (A8): custom `IGraphEdge` implementations must receive geometry notifications, `applyLayout()` must deliver `LayoutResult::edgeWaypoints`, and `GraphEdgeItem` must take its `EdgePathStrategy` as `std::unique_ptr`.

**Architecture:** Two small virtuals with safe defaults on `IGraphEdge` (`endpointsChanged()`, `setWaypoints()`); `GraphScene` dispatches through them instead of `dynamic_cast<GraphEdgeItem*>`. Raw-pointer strategy APIs stay as `[[deprecated]]` delegating overloads so external consumers keep compiling.

**Tech Stack:** C++20, Qt 6.8, QTest. Build dir `build/`. Prerequisite: Phase 1 merged (not strictly required, but the roadmap sequences it first).

---

### Task 1: `IGraphEdge::endpointsChanged()` — scene stops dynamic_casting (A1)

**Files:**
- Modify: `src/core/include/graffodil/IGraphEdge.h`
- Modify: `src/core/include/graffodil/GraphEdgeItem.h` (~line 36, IGraphEdge section)
- Modify: `src/core/src/GraphScene.cpp:390-407` (and drop the now-unneeded include)
- Test: `tests/tst_graphscene.cpp`

- [x] **Step 1: Write the failing test**

In `tests/tst_graphscene.cpp` (file scope, after includes — `TestHelpers.h` is
already included by this test):

```cpp
class AdjustRecordingEdge : public Graffodil::Test::TestEdge {
public:
    using TestEdge::TestEdge;
    void endpointsChanged() override { ++adjustCount; }
    int adjustCount = 0;
};
```

Test slot:

```cpp
void customEdgeReceivesEndpointsChanged()
{
    Graffodil::GraphScene scene;
    auto *a = new Graffodil::Test::TestNode("a", {0, 0});
    auto *b = new Graffodil::Test::TestNode("b", {200, 0});
    scene.addNode(a);
    scene.addNode(b);
    auto *edge = new AdjustRecordingEdge("e1", a, b);
    scene.addEdge(edge);

    scene.adjustAllEdges();
    QCOMPARE(edge->adjustCount, 1);

    scene.adjustEdgesForNode("a");
    QCOMPARE(edge->adjustCount, 2);

    scene.adjustEdgesForNode("unrelated-id");
    QCOMPARE(edge->adjustCount, 2);
}
```

- [x] **Step 2: Verify it fails to compile**

Run: `cmake --build build`
Expected: error — `endpointsChanged` is not a virtual of `TestEdge`/`IGraphEdge`
(`marked 'override', but does not override`).

- [x] **Step 3: Add the virtual and rewire the scene**

`src/core/include/graffodil/IGraphEdge.h` — inside the class, after
`targetNodeId()`:

```cpp
    /// Notification that either endpoint node may have moved or changed
    /// geometry. The scene calls this from adjustEdgesForNode(),
    /// adjustAllEdges(), and applyLayout(). Implementations should recompute
    /// their visual path. Default is a no-op so data-only edges stay valid.
    virtual void endpointsChanged() {}
```

`src/core/include/graffodil/GraphEdgeItem.h` — in the `// --- IGraphEdge ---`
block:

```cpp
    void endpointsChanged() override { adjust(); }
```

`src/core/src/GraphScene.cpp` — replace both methods at lines 390-407:

```cpp
void GraphScene::adjustEdgesForNode(const QString &nodeId)
{
    for (auto *edge : m_edges) {
        if (edge->sourceNode()->nodeId() == nodeId
            || edge->targetNode()->nodeId() == nodeId) {
            edge->endpointsChanged();
        }
    }
}

void GraphScene::adjustAllEdges()
{
    for (auto *edge : m_edges)
        edge->endpointsChanged();
}
```

Remove `#include <graffodil/GraphEdgeItem.h>` from `GraphScene.cpp` (line 6) —
the dynamic_cast was its only use. If the build then complains about another
use, leave it and note where.

- [x] **Step 4: Run tests**

Run: `cmake --build build && (cd build && ctest --output-on-failure)`
Expected: all PASS — the new slot, plus every existing test (GraphEdgeItem
edges still adjust because `endpointsChanged()` → `adjust()`).

- [x] **Step 5: Commit**

```bash
git add src/core tests/tst_graphscene.cpp
git commit -m "feat(core): IGraphEdge::endpointsChanged — custom edges get geometry notifications"
```

---

### Task 2: `applyLayout()` delivers edge waypoints (A2)

**Files:**
- Modify: `src/core/include/graffodil/IGraphEdge.h`
- Modify: `src/core/include/graffodil/GraphEdgeItem.h:43`
- Modify: `src/core/src/GraphScene.cpp:376-388` (`applyLayout`)
- Test: `tests/tst_graphscene.cpp`

- [x] **Step 1: Write the failing test**

```cpp
void applyLayoutDeliversWaypoints()
{
    using namespace Graffodil;
    GraphScene scene;
    auto *a = new Test::TestNode("a", {0, 0});
    auto *b = new Test::TestNode("b", {400, 0});
    scene.addNode(a);
    scene.addNode(b);
    auto *edge = new GraphEdgeItem(a, QString(), b, QString(),
                                   new PolylinePathStrategy);
    scene.addEdge(edge);

    LayoutResult result;
    result.nodePositions["a"] = QPointF(0, 0);
    result.nodePositions["b"] = QPointF(400, 0);
    result.edgeWaypoints[{QStringLiteral("a"), QStringLiteral("b")}] =
        {QPointF(200, 150)};
    scene.applyLayout(result);

    // The polyline must detour through the waypoint at y=150; without
    // delivery it is a straight line with y extent ~0.
    QVERIFY(edge->path().boundingRect().bottom() >= 149.0);
}
```

(Needs `#include <graffodil/GraphEdgeItem.h>` and
`#include <graffodil/EdgePathStrategy.h>` in the test file if not present.)

- [x] **Step 2: Run to verify it fails**

Run: `cmake --build build && (cd build && ctest -R tst_graphscene --output-on-failure)`
Expected: FAIL on the bounding-rect assertion (waypoints never delivered).

- [x] **Step 3: Implement**

`src/core/include/graffodil/IGraphEdge.h` — after `endpointsChanged()`:

```cpp
    /// Routing hints delivered by GraphScene::applyLayout() when a layout
    /// engine produced LayoutResult::edgeWaypoints for this edge's
    /// (sourceId, targetId) pair. Scene coordinates, ordered source→target.
    /// Default ignores them; GraphEdgeItem feeds them to its path strategy.
    virtual void setWaypoints(const QList<QPointF> &waypoints)
    {
        Q_UNUSED(waypoints);
    }
```

Add `#include <QList>` / `#include <QPointF>` to IGraphEdge.h's include block.

`src/core/include/graffodil/GraphEdgeItem.h` line 43 — mark the existing
method as the override:

```cpp
    void setWaypoints(const QList<QPointF> &waypoints) override;
```

`src/core/src/GraphScene.cpp` — `applyLayout` becomes:

```cpp
void GraphScene::applyLayout(const LayoutResult &result)
{
    for (auto it = result.nodePositions.constBegin();
         it != result.nodePositions.constEnd(); ++it) {
        if (auto *node = nodeForId(it.key()))
            node->graphicsItem()->setPos(it.value());
    }
    // Deliver routing hints before adjusting so the recomputed paths see
    // them. Keyed by (sourceId, targetId), direction-sensitive — matches
    // what the layout engines emit. Parallel edges between the same pair
    // share the first match (documented limitation of the key shape).
    for (auto it = result.edgeWaypoints.constBegin();
         it != result.edgeWaypoints.constEnd(); ++it) {
        if (auto *edge = edgeBetween(it.key().first, it.key().second))
            edge->setWaypoints(it.value());
    }
    adjustAllEdges();
    // Groups derive from member positions; refresh synchronously so
    // callers that query bounds right after applyLayout() see fresh data
    // instead of waiting for the next scene->changed emission.
    refreshGroups();
}
```

- [x] **Step 4: Run tests** — Expected: PASS, including `tst_sugiyama` and `tst_integration`.

- [x] **Step 5: Commit**

```bash
git add src/core tests/tst_graphscene.cpp
git commit -m "feat(core): applyLayout delivers LayoutResult::edgeWaypoints to edges"
```

---

### Task 3: unique_ptr strategy ownership with deprecated raw overloads (A8)

`GraphEdgeItem` takes `EdgePathStrategy*` raw and silently wraps it in a
`unique_ptr`, while `setTerminus` already takes `std::unique_ptr` — two
transfer styles in one class. Add `unique_ptr` overloads, deprecate the raw
ones (kept so Corbomite/PlanStan keep compiling), and migrate in-repo callers.

**Files:**
- Modify: `src/core/include/graffodil/GraphEdgeItem.h:27-29,39`
- Modify: `src/core/src/GraphEdgeItem.cpp:13-27,36-39`
- Modify (call sites): `demo/DemoWindow.cpp:103,393`, `tests/tst_graphscene.cpp:169,211`, `tests/tst_graphedgeitem.cpp:76`, `tests/tst_integration.cpp:34`, plus the Task-2 test added above (line numbers pre-Phase-2; re-locate with the grep below)

- [x] **Step 1: Add the overloads**

`src/core/include/graffodil/GraphEdgeItem.h` — replace the constructor
declaration and `setPathStrategy`:

```cpp
    GraphEdgeItem(IGraphNode *source, const QString &sourceAnchorId,
                  IGraphNode *target, const QString &targetAnchorId,
                  std::unique_ptr<EdgePathStrategy> strategy);

    [[deprecated("pass std::unique_ptr<EdgePathStrategy>; ownership transfers either way")]]
    GraphEdgeItem(IGraphNode *source, const QString &sourceAnchorId,
                  IGraphNode *target, const QString &targetAnchorId,
                  EdgePathStrategy *strategy);
```

```cpp
    void setPathStrategy(std::unique_ptr<EdgePathStrategy> strategy);
    [[deprecated("pass std::unique_ptr<EdgePathStrategy>; ownership transfers either way")]]
    void setPathStrategy(EdgePathStrategy *strategy);
```

`src/core/src/GraphEdgeItem.cpp` — the unique_ptr constructor takes over the
existing body (member init `m_strategy(std::move(strategy))`); the raw one
delegates:

```cpp
GraphEdgeItem::GraphEdgeItem(IGraphNode *source, const QString &sourceAnchorId,
                             IGraphNode *target, const QString &targetAnchorId,
                             std::unique_ptr<EdgePathStrategy> strategy)
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_source(source)
    , m_target(target)
    , m_sourceAnchorId(sourceAnchorId)
    , m_targetAnchorId(targetAnchorId)
    , m_strategy(std::move(strategy))
{
    setFlag(ItemIsSelectable);
    // Hovering an edge should signal that it's clickable.
    setCursor(Qt::PointingHandCursor);
    setAcceptHoverEvents(true);
}

GraphEdgeItem::GraphEdgeItem(IGraphNode *source, const QString &sourceAnchorId,
                             IGraphNode *target, const QString &targetAnchorId,
                             EdgePathStrategy *strategy)
    : GraphEdgeItem(source, sourceAnchorId, target, targetAnchorId,
                    std::unique_ptr<EdgePathStrategy>(strategy))
{
}

void GraphEdgeItem::setPathStrategy(std::unique_ptr<EdgePathStrategy> strategy)
{
    m_strategy = std::move(strategy);
}

void GraphEdgeItem::setPathStrategy(EdgePathStrategy *strategy)
{
    setPathStrategy(std::unique_ptr<EdgePathStrategy>(strategy));
}
```

- [x] **Step 2: Build — expect deprecation warnings at the in-repo call sites**

Run: `cmake --build build 2>&1 | grep -i deprecated`
Expected: warnings at each remaining raw-pointer call site (the repo builds;
warnings are not errors).

- [x] **Step 3: Migrate every in-repo call site**

Locate them:

```bash
grep -rn 'new GraphEdgeItem\|setPathStrategy(' demo/ tests/ src/
```

For each constructor site, wrap the strategy argument in
`std::make_unique<...>()`. E.g. `demo/DemoWindow.cpp:393`:

```cpp
    auto *e = new GraphEdgeItem(src, srcId, tgt, tgtId,
                                std::make_unique<BezierPathStrategy>());
```

Apply the same mechanical change at `demo/DemoWindow.cpp:103`,
`tests/tst_graphscene.cpp:169,211` (and the Task-2 waypoint test),
`tests/tst_graphedgeitem.cpp:76`, `tests/tst_integration.cpp:34` — in each
case the existing `new XxxPathStrategy(...)` argument becomes
`std::make_unique<XxxPathStrategy>(...)` with identical constructor
arguments. Where a site passes a pre-built strategy variable instead of an
inline `new`, wrap it: `std::unique_ptr<EdgePathStrategy>(strategy)`. Add
`#include <memory>` where missing.

- [x] **Step 4: Verify zero deprecation warnings and green tests**

Run: `cmake --build build 2>&1 | grep -i deprecated` → no output.
Run: `(cd build && ctest --output-on-failure)` → all PASS.

- [x] **Step 5: Commit**

```bash
git add src/core demo tests
git commit -m "refactor(core): GraphEdgeItem takes EdgePathStrategy as unique_ptr (raw overloads deprecated)"
```

---

### Task 4: Close out the phase

- [x] Update `docs/graffodil-design.md` §IGraphEdge: document the two new
      virtuals (copy the doc comments from the header) and the waypoint
      delivery behavior of `applyLayout()`, including the parallel-edge
      limitation.
- [x] Update `docs/ROADMAP.md`: Phase 2 → ☑ with commit hash; Status-log line.
- [x] `git add docs && git commit -m "docs: mark Phase 2 (edge contract) complete"`
