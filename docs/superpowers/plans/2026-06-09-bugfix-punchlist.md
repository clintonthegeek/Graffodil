# Bugfix Punch List Implementation Plan (Phase 1)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix all six confirmed bugs from the 2026-06-09 evaluation (findings B1–B6 in `docs/evaluation-2026-06-09.md`): two ForceLayout math bugs, missing key-release forwarding, the documented minimap bug, a dead-code vestige, and the batch items' hardcoded bounding rects.

**Architecture:** Six independent, surgical fixes. No API breaks except one private member rename. Each task is self-contained and individually committable.

**Tech Stack:** C++20, Qt 6.8, QTest, CMake. Build dir `build/`.

**Build/test commands used throughout:**
```bash
cmake --build build
(cd build && ctest --output-on-failure)            # all tests
(cd build && ctest -R <name> --output-on-failure)  # one test
```
If `build/` doesn't exist: `cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`

---

### Task 1: ForceLayout — energy convergence detects plateau, not decrease (B2)

The current code (`src/force/src/ForceLayout.cpp:480-489`) declares convergence
after energy has *decreased* for 10 consecutive ticks — i.e. exactly while the
simulation is still actively improving. Convergence is when energy *stops
changing*.

**Files:**
- Modify: `src/force/include/graffodil/ForceLayout.h:61`
- Modify: `src/force/src/ForceLayout.cpp:479-489`
- Test: `tests/tst_forcelayout.cpp`

- [ ] **Step 1: Write the failing test**

Add to the test class in `tests/tst_forcelayout.cpp` (it already includes
`TestHelpers.h` and constructs node/edge lists; follow the existing slot
style):

```cpp
void energyConvergenceIsPlateauNotDecrease()
{
    using namespace Graffodil;
    // A 20-node chain takes well over 15 ticks to physically settle from
    // BFS initial placement. The pre-fix heuristic stops after ~10 ticks of
    // monotonically decreasing energy — long before the layout has settled.
    QList<Test::TestNode *> nodeStore;
    QList<IGraphNode *> nodes;
    QList<IGraphEdge *> edges;
    QList<Test::TestEdge *> edgeStore;
    for (int i = 0; i < 20; ++i) {
        auto *n = new Test::TestNode(QString("n%1").arg(i));
        nodeStore.append(n);
        nodes.append(n);
    }
    for (int i = 0; i + 1 < 20; ++i) {
        auto *e = new Test::TestEdge(QString("e%1").arg(i),
                                     nodes[i], nodes[i + 1]);
        edgeStore.append(e);
        edges.append(e);
    }

    ForceLayout layout;
    layout.setNodes(nodes);
    layout.setEdges(edges);

    int ticks = 0;
    while (layout.tick() && ticks < 5000)
        ++ticks;

    QVERIFY(layout.isConverged());
    QVERIFY2(ticks > 15,
             "simulation stopped while energy was still actively decreasing");

    qDeleteAll(edgeStore);
    qDeleteAll(nodeStore);
}
```

Register the slot in the test class's `private slots:` section.

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && (cd build && ctest -R tst_forcelayout --output-on-failure)`
Expected: FAIL on `ticks > 15` (pre-fix it converges after ~10-11 ticks).
If it unexpectedly passes, check whether the displacement fallback converged
first — bump the chain to 40 nodes before concluding the test is wrong.

- [ ] **Step 3: Apply the fix**

In `src/force/include/graffodil/ForceLayout.h`, line 61, rename the counter:

```cpp
    int m_energyStableCount = 0;
```
(was `int m_energyDecreaseCount = 0;`)

In `src/force/src/ForceLayout.cpp`, replace the energy-based convergence block
(lines 479-489):

```cpp
    // 8. Convergence detection

    // Energy-plateau convergence: converged when energy stops changing
    // run-over-run. (A correctly cooling simulation *decreases* energy every
    // tick — decreasing energy means still improving, not converged.)
    if (m_prevEnergy > EPSILON) {
        const double relChange =
            std::abs(m_energy - m_prevEnergy) / m_prevEnergy;
        if (relChange < 0.001) {
            ++m_energyStableCount;
            if (m_energyStableCount >= 10)
                m_converged = true;
        } else {
            m_energyStableCount = 0;
        }
    }
```

(`<cmath>` is already in use in this file via `std::sqrt`; add
`#include <cmath>` at the top if it isn't explicitly there.)

- [ ] **Step 4: Run tests**

Run: `cmake --build build && (cd build && ctest -R tst_forcelayout --output-on-failure)`
Expected: PASS, including all pre-existing force tests.

- [ ] **Step 5: Commit**

```bash
git add src/force tests/tst_forcelayout.cpp
git commit -m "fix(force): energy convergence detects plateau, not decrease"
```

---

### Task 2: ForceLayout — adaptive speed cap was a no-op (B1)

`src/force/src/ForceLayout.cpp:436` reads
`m_globalSpeed = std::min(m_globalSpeed, m_globalSpeed * 1.5);` — for positive
values `min(x, 1.5x) == x`, so the ForceAtlas2 speed-growth cap never applies.
The cap must compare against the *previous* iteration's speed.

This is a numeric correction whose failure mode (speed spikes when global
swinging momentarily collapses) is not deterministic enough for a
fail-first unit test. Verification is: a bounded-positions regression guard
plus the full existing suite.

**Files:**
- Modify: `src/force/src/ForceLayout.cpp:432-437`
- Test: `tests/tst_forcelayout.cpp`

- [ ] **Step 1: Write the regression-guard test** (passes before AND after — it pins the invariant)

```cpp
void positionsStayFiniteAndBounded()
{
    using namespace Graffodil;
    // Star graph — high-swinging scenario that exercises the speed cap.
    QList<Test::TestNode *> nodeStore;
    QList<IGraphNode *> nodes;
    QList<IGraphEdge *> edges;
    QList<Test::TestEdge *> edgeStore;
    auto *hub = new Test::TestNode("hub");
    nodeStore.append(hub);
    nodes.append(hub);
    for (int i = 0; i < 30; ++i) {
        auto *n = new Test::TestNode(QString("leaf%1").arg(i));
        nodeStore.append(n);
        nodes.append(n);
        auto *e = new Test::TestEdge(QString("e%1").arg(i), hub, n);
        edgeStore.append(e);
        edges.append(e);
    }

    ForceLayout layout;
    layout.setNodes(nodes);
    layout.setEdges(edges);
    for (int i = 0; i < 300 && layout.tick(); ++i) {}

    const LayoutResult r = layout.currentPositions();
    for (auto it = r.nodePositions.constBegin();
         it != r.nodePositions.constEnd(); ++it) {
        QVERIFY(std::isfinite(it.value().x()));
        QVERIFY(std::isfinite(it.value().y()));
        QVERIFY(qAbs(it.value().x()) < 1.0e6);
        QVERIFY(qAbs(it.value().y()) < 1.0e6);
    }

    qDeleteAll(edgeStore);
    qDeleteAll(nodeStore);
}
```

- [ ] **Step 2: Run it** — Expected: PASS (it guards against future regressions).

- [ ] **Step 3: Apply the fix**

Replace lines 432-437 of `src/force/src/ForceLayout.cpp`:

```cpp
    double tolerance = 1.0;
    const double previousSpeed = m_globalSpeed;
    if (globalSwinging > EPSILON)
        m_globalSpeed = tolerance * globalTraction / globalSwinging;

    // ForceAtlas2 rule: cap growth at 1.5x the previous iteration's speed.
    // (The old expression min(x, x*1.5) compared the new speed to itself
    // and was a no-op.)
    m_globalSpeed = std::min(m_globalSpeed, previousSpeed * 1.5);
    m_globalSpeed = std::max(m_globalSpeed, EPSILON);
```

- [ ] **Step 4: Run the full force + integration suites**

Run: `cmake --build build && (cd build && ctest -R 'forcelayout|integration' --output-on-failure)`
Expected: PASS. (If `energyConvergenceIsPlateauNotDecrease` from Task 1 now
needs more ticks, that is expected and fine — it asserts a lower bound only.)

- [ ] **Step 5: Commit**

```bash
git add src/force tests/tst_forcelayout.cpp
git commit -m "fix(force): cap adaptive speed against previous tick's speed"
```

---

### Task 3: GraphScene forwards keyReleaseEvent to the active tool (B3)

**Files:**
- Modify: `src/core/include/graffodil/GraphScene.h:145` (protected overrides block)
- Modify: `src/core/src/GraphScene.cpp` (after `keyPressEvent`, ~line 459)
- Test: `tests/tst_graphscene.cpp`

- [ ] **Step 1: Write the failing test**

In `tests/tst_graphscene.cpp`, add a minimal recording tool near the top of
the file (file scope, after includes):

```cpp
class KeyRecordingTool : public Graffodil::GraphTool {
public:
    void mousePressEvent(QGraphicsSceneMouseEvent *) override {}
    void mouseMoveEvent(QGraphicsSceneMouseEvent *) override {}
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *) override {}
    void keyReleaseEvent(QKeyEvent *) override { releasedCount++; }
    int releasedCount = 0;
};
```

(Add `#include <graffodil/GraphTool.h>` and `#include <QKeyEvent>` if not
already present.) Then the test slot:

```cpp
void keyReleaseReachesActiveTool()
{
    Graffodil::GraphScene scene;
    KeyRecordingTool tool;
    scene.setActiveTool(&tool);

    QKeyEvent ev(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier);
    QCoreApplication::sendEvent(&scene, &ev);

    QCOMPARE(tool.releasedCount, 1);
    scene.setActiveTool(nullptr);  // detach before tool goes out of scope
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && (cd build && ctest -R tst_graphscene --output-on-failure)`
Expected: FAIL — `releasedCount` is 0 (QGraphicsScene swallows the release).

- [ ] **Step 3: Implement**

`src/core/include/graffodil/GraphScene.h`, in the `protected:` block directly
after `void keyPressEvent(QKeyEvent *e) override;`:

```cpp
    void keyReleaseEvent(QKeyEvent *e) override;
```

`src/core/src/GraphScene.cpp`, directly after the `keyPressEvent` definition:

```cpp
void GraphScene::keyReleaseEvent(QKeyEvent *e)
{
    if (m_activeTool)
        m_activeTool->keyReleaseEvent(e);
    else
        QGraphicsScene::keyReleaseEvent(e);
}
```

- [ ] **Step 4: Run tests** — Expected: PASS (`tst_graphscene` and `tst_tools`).

- [ ] **Step 5: Commit**

```bash
git add src/core tests/tst_graphscene.cpp
git commit -m "fix(core): forward keyReleaseEvent to the active tool"
```

---

### Task 4: Minimap uses authored sceneRect with viewport union (B4)

Implements the fix specified in
`docs/bugs/minimap-uses-itemsBoundingRect-not-sceneRect.md`. The rect
computation is extracted into a public static helper so it is testable without
pixel inspection, and so `paintEvent` and `mapToScene` can never drift apart
again.

**Files:**
- Modify: `src/core/include/graffodil/GraphMinimap.h`
- Modify: `src/core/src/GraphMinimap.cpp:46-49,116-119`
- Create: `tests/tst_minimap.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Create the failing test file**

`tests/tst_minimap.cpp`:

```cpp
#include <graffodil/GraphMinimap.h>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QtTest>

using namespace Graffodil;

class TestMinimap : public QObject {
    Q_OBJECT
private slots:
    void authoredSceneRectWins()
    {
        QGraphicsScene scene;
        scene.addItem(new QGraphicsRectItem(0, 0, 100, 100));
        scene.setSceneRect(0, 0, 1000, 1000);
        QGraphicsView view(&scene);
        view.resize(200, 200);

        const QRectF r = GraphMinimap::effectiveSceneRect(&view);
        // Must reflect the authored 1000x1000 canvas, not the 100x100 items.
        QVERIFY(r.width() >= 1000.0);
        QVERIFY(r.height() >= 1000.0);
    }

    void viewportAlwaysContained()
    {
        QGraphicsScene scene;
        scene.addItem(new QGraphicsRectItem(0, 0, 100, 100));
        scene.setSceneRect(0, 0, 1000, 1000);
        QGraphicsView view(&scene);
        view.resize(200, 200);
        view.centerOn(900, 900);   // pan far from the items

        const QRectF viewport =
            view.mapToScene(view.viewport()->rect()).boundingRect();
        const QRectF r = GraphMinimap::effectiveSceneRect(&view);
        QVERIFY(r.contains(viewport));
    }

    void emptySceneYieldsNullRect()
    {
        QGraphicsScene scene;   // no items, no authored rect
        QGraphicsView view(&scene);
        QVERIFY(GraphMinimap::effectiveSceneRect(&view).isNull());
        QVERIFY(GraphMinimap::effectiveSceneRect(nullptr).isNull());
    }
};

QTEST_MAIN(TestMinimap)
#include "tst_minimap.moc"
```

Append to `tests/CMakeLists.txt`:

```cmake
add_executable(tst_minimap tst_minimap.cpp)
target_link_libraries(tst_minimap PRIVATE Graffodil::Core Qt6::Test)
add_test(NAME tst_minimap COMMAND tst_minimap)
```

- [ ] **Step 2: Reconfigure and verify it fails to build**

Run: `cmake -S . -B build && cmake --build build`
Expected: compile error — `effectiveSceneRect` doesn't exist yet.

- [ ] **Step 3: Implement**

`src/core/include/graffodil/GraphMinimap.h` — in the `public:` section after
`void setEnabled(bool visible);`:

```cpp
    /// The scene region the minimap represents: the scene's sceneRect()
    /// (authored or auto-grown) when non-empty, otherwise the items bounding
    /// rect plus a 50px halo; in both cases unioned with the view's current
    /// viewport so the viewport indicator can never escape the minimap.
    /// Null rect when there is nothing to show. Static for testability.
    static QRectF effectiveSceneRect(QGraphicsView *view);
```

`src/core/src/GraphMinimap.cpp` — add the definition (after the constructor):

```cpp
QRectF GraphMinimap::effectiveSceneRect(QGraphicsView *view)
{
    if (!view || !view->scene())
        return {};
    QRectF rect = view->scene()->sceneRect();
    if (rect.isEmpty()) {
        rect = view->scene()->itemsBoundingRect();
        if (rect.isNull())
            return {};
        rect.adjust(-50, -50, 50, 50);
    }
    return rect.united(
        view->mapToScene(view->viewport()->rect()).boundingRect());
}
```

In `paintEvent`, replace lines 46-49:

```cpp
    QRectF sceneRect = effectiveSceneRect(m_graphicsView);
    if (sceneRect.isNull())
        return;
```

In `mapToScene`, replace lines 116-119:

```cpp
    QRectF sceneRect = effectiveSceneRect(m_graphicsView);
    if (sceneRect.isNull())
        return {};
```

(The duplicated `-50/-50/50/50` adjusts at both former sites are now inside
the helper — make sure neither call site still applies its own halo.)

- [ ] **Step 4: Run tests**

Run: `cmake --build build && (cd build && ctest -R tst_minimap --output-on-failure)`
Expected: PASS, all three slots.

- [ ] **Step 5: Update the bug doc and commit**

Append to `docs/bugs/minimap-uses-itemsBoundingRect-not-sceneRect.md`:

```markdown

## Resolution (2026-06-09 session or later)

Fixed as specified: extents now come from
`GraphMinimap::effectiveSceneRect()` — authored `sceneRect()` first, items
bbox + halo fallback, viewport union in both `paintEvent` and `mapToScene`.
Covered by `tests/tst_minimap.cpp`.
```

```bash
git add src/core tests/tst_minimap.cpp tests/CMakeLists.txt docs/bugs/
git commit -m "fix(core): minimap respects authored sceneRect, viewport never escapes"
```

---

### Task 5: Remove the setEdgesAboveNodes z-vestige (B6)

**Files:**
- Modify: `src/core/src/GraphScene.cpp:100-104`

- [ ] **Step 1: Delete the dead code**

In `GraphScene::setEdgesAboveNodes`, delete these lines:

```cpp
    // Re-assign z on items still at the prior defaults.
    const qreal prevNodeZ = above ? DefaultNodeZ : DefaultEdgeZ;  // new meaning
    const qreal prevEdgeZ = above ? DefaultEdgeZ : DefaultNodeZ;
    Q_UNUSED(prevNodeZ);
    Q_UNUSED(prevEdgeZ);
```

and replace with the single accurate comment:

```cpp
    // Re-assign z on items still at either default value.
```

- [ ] **Step 2: Build and run the full suite**

Run: `cmake --build build && (cd build && ctest --output-on-failure)`
Expected: PASS — behavior is unchanged (the deleted values were never read).

- [ ] **Step 3: Commit**

```bash
git add src/core/src/GraphScene.cpp
git commit -m "chore(core): drop unused prev-z locals in setEdgesAboveNodes"
```

---

### Task 6: Tight bounding rects for batch items (B5)

`BatchNodeItem`/`BatchEdgeItem` return a hardcoded ±10,000 rect: graphs
beyond it get clipped, and every `update()` dirties the entire region.
Additionally `updatePositions()`/`updateLines()` change geometry without
`prepareGeometryChange()` — harmless with a constant rect, mandatory with a
tight one.

**Files:**
- Modify: `src/batch/src/BatchNodeItem.h`, `src/batch/src/BatchNodeItem.cpp`
- Modify: `src/batch/src/BatchEdgeItem.h`, `src/batch/src/BatchEdgeItem.cpp`
- Test: `tests/tst_batchrenderer.cpp`, `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing tests**

These items live in private headers, so give the test target access. In
`tests/CMakeLists.txt`, extend the tst_batchrenderer entry:

```cmake
target_include_directories(tst_batchrenderer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/batch/src)
```

In `tests/tst_batchrenderer.cpp` add (consult `BatchNodeItem.h` /
`BatchEdgeItem.h` for the exact `NodeData`/`EdgeData` field names — `position`,
`radius` and `line` are the ones used below):

```cpp
#include "BatchNodeItem.h"
#include "BatchEdgeItem.h"

void tightNodeBounds()
{
    Graffodil::BatchNodeItem item;
    QVector<Graffodil::BatchNodeItem::NodeData> nodes(2);
    nodes[0].position = QPointF(0, 0);
    nodes[0].radius = 10;
    nodes[1].position = QPointF(15000, 300);   // beyond the old ±10000 rect
    nodes[1].radius = 10;
    item.setNodes(nodes);

    const QRectF b = item.boundingRect();
    QVERIFY(b.contains(QPointF(0, 0)));
    QVERIFY(b.contains(QPointF(15000, 300)));   // old code clipped this node
    QVERIFY(b.width() < 16000.0);                // and was 20000 wide regardless
}

void tightEdgeBounds()
{
    Graffodil::BatchEdgeItem item;
    QVector<Graffodil::BatchEdgeItem::EdgeData> edges(1);
    edges[0].line = QLineF(QPointF(0, 0), QPointF(12000, 50));
    item.setEdges(edges);

    const QRectF b = item.boundingRect();
    QVERIFY(b.contains(QPointF(12000, 50)));
    QVERIFY(b.height() < 200.0);   // a near-horizontal line, not a 20000px band
}
```

Register both slots in the test class.

- [ ] **Step 2: Run to verify failure**

Run: `cmake -S . -B build && cmake --build build && (cd build && ctest -R tst_batchrenderer --output-on-failure)`
Expected: FAIL — `contains(15000,300)` false and width 20000 / height 20000.

- [ ] **Step 3: Implement BatchNodeItem**

`src/batch/src/BatchNodeItem.h`: add to the private section
`QRectF m_bounds;` and declare `void recomputeBounds();`.

`src/batch/src/BatchNodeItem.cpp`:

```cpp
void BatchNodeItem::recomputeBounds()
{
    QRectF b;
    for (const auto &node : m_nodes) {
        qreal rx = node.radius, ry = node.radius;
        if (!node.cachedPixmap.isNull() && node.cachedSize.isValid()) {
            rx = qMax(rx, node.cachedSize.width() / 2.0);
            ry = qMax(ry, node.cachedSize.height() / 2.0);
        }
        b |= QRectF(node.position.x() - rx, node.position.y() - ry,
                    rx * 2.0, ry * 2.0);
    }
    m_bounds = b;
}

void BatchNodeItem::setNodes(const QVector<NodeData> &nodes)
{
    prepareGeometryChange();
    m_nodes = nodes;
    recomputeBounds();
    update();
}

void BatchNodeItem::updatePositions(const QVector<QPointF> &positions)
{
    prepareGeometryChange();
    int count = qMin(positions.size(), m_nodes.size());
    for (int i = 0; i < count; ++i)
        m_nodes[i].position = positions[i];
    recomputeBounds();
    update();
}

QRectF BatchNodeItem::boundingRect() const
{
    return m_bounds;
}
```

- [ ] **Step 4: Implement BatchEdgeItem**

`src/batch/src/BatchEdgeItem.h`: add `QRectF m_bounds;` and
`void recomputeBounds();` to the private section.

`src/batch/src/BatchEdgeItem.cpp` — same pattern; the 1px margin covers the
cosmetic pen width:

```cpp
void BatchEdgeItem::recomputeBounds()
{
    QRectF b;
    for (const auto &e : m_edges)
        b |= QRectF(e.line.p1(), e.line.p2()).normalized();
    m_bounds = b.isNull() ? b : b.adjusted(-1.0, -1.0, 1.0, 1.0);
}
```

In `setEdges`: after the `std::partition` assignment block, call
`recomputeBounds();` before `update();`.
In `updateLines`: add `prepareGeometryChange();` as the first line and
`recomputeBounds();` before `update();`.
Replace `boundingRect()` to `return m_bounds;`.

- [ ] **Step 5: Run tests**

Run: `cmake --build build && (cd build && ctest -R tst_batchrenderer --output-on-failure)`
Expected: PASS (new slots and all pre-existing batch tests).

- [ ] **Step 6: Full suite + commit**

Run: `(cd build && ctest --output-on-failure)` — Expected: all PASS.

```bash
git add src/batch tests/
git commit -m "fix(batch): tight bounding rects for batch node/edge items"
```

---

### Task 7: Close out the phase

- [ ] Update `docs/ROADMAP.md`: set Phase 1 row to ☑ with the final commit hash; add a Status-log line.
- [ ] Run the demo (`./build/demo/graffodil-demo` or the binary under `build/demo/`) and sanity-check: force layout settles smoothly, minimap viewport stays inside chrome while panning, batch mode still renders.
- [ ] `git add docs/ROADMAP.md && git commit -m "docs: mark Phase 1 (bugfix punch list) complete"`
