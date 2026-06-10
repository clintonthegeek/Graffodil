# Hot-Path Caching Implementation Plan (Phase 4)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate repeated expensive recomputation in the two hottest paths (evaluation findings P1, P2, P3): `GraphEdgeItem::paint()` re-measuring the path every frame, `GraphEdgeItem::shape()` re-stroking on every hit test, and `ForceLayout::tick()` doing ~6 QString-hash lookups per node per tick.

**Architecture:** Pure caching — no behavior change. Edge path metrics (length, endpoints, tangent reference points) are computed once in `adjust()` where the path is rebuilt; the hit shape is cached lazily and invalidated on path/hit-width change. ForceLayout precomputes a mass-by-index vector in `buildInternalStructures()`.

**Tech Stack:** C++20, Qt 6.8, QTest. Build dir `build/`.
**Prerequisite:** Phase 2 merged (it touches the same `GraphEdgeItem` region; rebase pain otherwise).

---

### Task 1: Cache path metrics + hit shape in GraphEdgeItem (P1, P2)

**Files:**
- Modify: `src/core/include/graffodil/GraphEdgeItem.h` (private members)
- Modify: `src/core/src/GraphEdgeItem.cpp` (`adjust`, `shape`, `setHitWidth`, `paint`)
- Test: `tests/tst_graphedgeitem.cpp`

- [x] **Step 1: Write the cache-invalidation tests**

The risk in this task is a stale cache, so the tests pin invalidation, not
speed. Add to `tests/tst_graphedgeitem.cpp` (it already builds nodes/edges —
follow its idiom; post-Phase-2 the strategy is passed as
`std::make_unique<...>()`):

```cpp
void shapeFollowsAdjust()
{
    using namespace Graffodil;
    auto *src = new Test::TestNode("s", {0, 0});
    auto *tgt = new Test::TestNode("t", {300, 0});
    auto *edge = new GraphEdgeItem(src, QString(), tgt, QString(),
                                   std::make_unique<DirectPathStrategy>());
    edge->adjust();
    const QRectF before = edge->shape().boundingRect();

    tgt->setPos(300, 400);     // move the target far down
    edge->adjust();
    const QRectF after = edge->shape().boundingRect();

    QVERIFY(after != before);
    QVERIFY(after.bottom() > before.bottom() + 100.0);

    delete edge; delete src; delete tgt;
}

void shapeFollowsHitWidth()
{
    using namespace Graffodil;
    auto *src = new Test::TestNode("s", {0, 0});
    auto *tgt = new Test::TestNode("t", {300, 0});
    auto *edge = new GraphEdgeItem(src, QString(), tgt, QString(),
                                   std::make_unique<DirectPathStrategy>());
    edge->adjust();
    const qreal narrow = edge->shape().boundingRect().height();
    edge->setHitWidth(40.0);
    const qreal wide = edge->shape().boundingRect().height();
    QVERIFY(wide > narrow + 10.0);

    delete edge; delete src; delete tgt;
}
```

- [x] **Step 2: Run them** — Expected: PASS pre-change (they pin behavior the
cache must preserve). Run:
`cmake --build build && (cd build && ctest -R tst_graphedgeitem --output-on-failure)`

- [x] **Step 3: Add the cache members**

`src/core/include/graffodil/GraphEdgeItem.h`, private section after
`m_gradient`:

```cpp
    // --- Caches (computed in adjust(); see docs in the cpp) ---
    qreal m_pathLength = 1.0;
    QPointF m_srcPoint, m_tgtPoint;            // path endpoints
    QPointF m_srcTangentRef, m_tgtTangentRef;  // terminus orientation points
    mutable QPainterPath m_shapeCache;
    mutable bool m_shapeDirty = true;
```

- [x] **Step 4: Populate in adjust(), invalidate in setHitWidth()**

`src/core/src/GraphEdgeItem.cpp` — `adjust()` becomes:

```cpp
void GraphEdgeItem::adjust()
{
    if (!m_strategy || !m_source || !m_target)
        return;

    prepareGeometryChange();

    const Anchor src = resolveSourceAnchor();
    const Anchor tgt = resolveTargetAnchor();

    QPainterPath path = m_strategy->computePath(src, tgt, m_waypoints);
    setPath(path);

    // Cache the metrics paint() and shape() need. QPainterPath::length()
    // and pointAtPercent() flatten the curve on every call — far too
    // expensive per-frame. NOTE: setPath() must never be called directly on
    // this item (it's inherited public from QGraphicsPathItem); always go
    // through adjust(), or these caches go stale.
    m_shapeDirty = true;
    if (path.isEmpty()) {
        m_pathLength = 1.0;
        m_srcPoint = m_tgtPoint = m_srcTangentRef = m_tgtTangentRef = QPointF();
        return;
    }
    m_pathLength = qMax(path.length(), 1.0);
    m_srcPoint = path.pointAtPercent(0.0);
    m_tgtPoint = path.pointAtPercent(1.0);
    m_srcTangentRef = path.pointAtPercent(qMin(0.05, 10.0 / m_pathLength));
    m_tgtTangentRef = path.pointAtPercent(qMax(0.95, 1.0 - 10.0 / m_pathLength));
}
```

`shape()` becomes a lazy cached build:

```cpp
QPainterPath GraphEdgeItem::shape() const
{
    if (path().isEmpty())
        return {};
    if (m_shapeDirty) {
        QPainterPathStroker stroker;
        stroker.setWidth(m_hitWidth);
        stroker.setCapStyle(Qt::RoundCap);
        stroker.setJoinStyle(Qt::RoundJoin);
        m_shapeCache = stroker.createStroke(path());
        m_shapeDirty = false;
    }
    return m_shapeCache;
}
```

`setHitWidth()` gains the invalidation:

```cpp
void GraphEdgeItem::setHitWidth(qreal width)
{
    if (qFuzzyCompare(m_hitWidth, width))
        return;
    prepareGeometryChange();
    m_hitWidth = width;
    m_shapeDirty = true;
}
```

- [x] **Step 5: Use the caches in paint()**

In `paint()`, replace the four per-frame path queries (lines ~191-194 and the
two terminus blocks):

```cpp
    const QPainterPath &p = path();
    const QPointF srcPos = m_srcPoint;
    const QPointF tgtPos = m_tgtPoint;
```

and the terminus blocks:

```cpp
    if (m_sourceTerminus)
        m_sourceTerminus->draw(painter, srcPos, m_srcTangentRef, edgePen);

    if (m_targetTerminus)
        m_targetTerminus->draw(painter, tgtPos, m_tgtTangentRef, edgePen);
```

(The `len` local disappears; the gradient block keeps using
`srcPos`/`tgtPos`.)

- [x] **Step 6: Full suite**

Run: `cmake --build build && (cd build && ctest --output-on-failure)`
Expected: all PASS — `tst_graphedgeitem`, `tst_terminus` (arrowheads still
oriented correctly), `tst_tools` (hit-testing via shape still works), plus the
two new slots.

- [x] **Step 7: Commit**

```bash
git add src/core tests/tst_graphedgeitem.cpp
git commit -m "perf(core): cache GraphEdgeItem path metrics and hit shape"
```

---

### Task 2: ForceLayout mass-by-index vector (P3)

`tick()` calls `m_degree.value(m_ids[i], 0)` — a QString-keyed hash lookup —
at six sites, several inside per-node loops that run every tick.

**Files:**
- Modify: `src/force/include/graffodil/ForceLayout.h` (member, ~line 52)
- Modify: `src/force/src/ForceLayout.cpp` (`buildInternalStructures`, `tick`)
- Test: `tests/tst_forcelayout.cpp`

- [x] **Step 1: Write the determinism guard**

The refactor must not change a single computed force. Pin it:

```cpp
void tickIsDeterministicAcrossInstances()
{
    using namespace Graffodil;
    auto build = [](QList<Test::TestNode *> &nodeStore,
                    QList<Test::TestEdge *> &edgeStore,
                    QList<IGraphNode *> &nodes, QList<IGraphEdge *> &edges) {
        for (int i = 0; i < 12; ++i) {
            auto *n = new Test::TestNode(QString("n%1").arg(i));
            nodeStore.append(n);
            nodes.append(n);
        }
        for (int i = 0; i < 12; ++i) {            // ring + one chord
            auto *e = new Test::TestEdge(QString("e%1").arg(i),
                                         nodes[i], nodes[(i + 1) % 12]);
            edgeStore.append(e);
            edges.append(e);
        }
        auto *chord = new Test::TestEdge("chord", nodes[0], nodes[6]);
        edgeStore.append(chord);
        edges.append(chord);
    };

    QList<Test::TestNode *> ns1, ns2;
    QList<Test::TestEdge *> es1, es2;
    QList<IGraphNode *> n1, n2;
    QList<IGraphEdge *> e1, e2;
    build(ns1, es1, n1, e1);
    build(ns2, es2, n2, e2);

    ForceLayout l1, l2;
    l1.setNodes(n1); l1.setEdges(e1);
    l2.setNodes(n2); l2.setEdges(e2);
    for (int i = 0; i < 50; ++i) { l1.tick(); l2.tick(); }

    const auto p1 = l1.currentPositions().nodePositions;
    const auto p2 = l2.currentPositions().nodePositions;
    QCOMPARE(p1.size(), p2.size());
    for (auto it = p1.constBegin(); it != p1.constEnd(); ++it)
        QCOMPARE(it.value(), p2.value(it.key()));

    qDeleteAll(es1); qDeleteAll(es2); qDeleteAll(ns1); qDeleteAll(ns2);
}
```

- [x] **Step 2: Run it twice** — once now (must PASS — establishes the
baseline is deterministic), and re-run after Step 4 (must still PASS —
the refactor changed nothing). Run:
`cmake --build build && (cd build && ctest -R tst_forcelayout --output-on-failure)`
If it FAILS *now*, the engine has hidden nondeterminism — stop and
investigate before proceeding (do not write the cache on a flaky baseline).

- [x] **Step 3: Build the vector**

`src/force/include/graffodil/ForceLayout.h` — next to `m_degree` (~line 51):

```cpp
    QVector<double> m_nodeMass;        // degree + 1, by node index (tick() hot path)
```

`src/force/src/ForceLayout.cpp` — at the end of `buildInternalStructures()`
(after the edge loop that fills `m_degree`):

```cpp
    m_nodeMass.resize(n);
    for (int i = 0; i < n; ++i)
        m_nodeMass[i] = m_degree.value(m_ids[i], 0) + 1.0;
```

- [x] **Step 4: Replace the six tick() lookups**

In `tick()`, replace every `m_degree.value(m_ids[i], 0) + 1.0` (and the `j`
variant) with the vector read. Sites (pre-change line numbers):

- L364 (Barnes-Hut masses loop): `masses[i] = m_nodeMass[i];`
- L370: `double nodeMass = m_nodeMass[i];`
- L378: `double degI = m_nodeMass[i];`
- L384: `double degJ = m_nodeMass[j];`
- L409 (center force): `double deg = m_nodeMass[i];`
- L418 (swinging): `double deg = m_nodeMass[i];`
- L446 (apply displacement): `double deg = m_nodeMass[i];`

Leave `m_degree` itself and its uses *outside* `tick()` (BFS placement,
`estimateCanvasArea`, etc.) untouched — this task only de-hashes the hot loop.

- [x] **Step 5: Verify determinism + full suite**

Run: `cmake --build build && (cd build && ctest --output-on-failure)`
Expected: all PASS, including `tickIsDeterministicAcrossInstances` — and
crucially, the Phase-1 convergence/boundedness tests still pass (the same
numbers are being computed, just fetched cheaper).

- [x] **Step 6: Commit**

```bash
git add src/force tests/tst_forcelayout.cpp
git commit -m "perf(force): index-based node mass vector replaces per-tick QString hash lookups"
```

---

### Task 3: Close out the phase

- [x] Optional spot-check, demo: load the largest demo graph, drag nodes and
      hover edges — interaction should feel at least as smooth as before.
- [x] Update `docs/ROADMAP.md`: Phase 4 → ☑ with commit hash; Status-log line.
- [x] `git add docs/ROADMAP.md && git commit -m "docs: mark Phase 4 (hot-path caching) complete"`
