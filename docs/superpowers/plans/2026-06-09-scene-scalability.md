# Scene Scalability Implementation Plan (Phase 3)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Make per-node edge lookups O(degree) instead of O(E) via an adjacency index (evaluation finding A4), and stop the group-refresh from running on every `QGraphicsScene::changed` emission (A5).

**Architecture:** A `QMultiHash<QString, IGraphEdge*>` maintained in `addEdge`/`removeEdge` backs `connectedEdges`/`adjustEdgesForNode`/`edgeBetween*`/`removeNode`. Group refresh stays driven by `changed` (so consumer-side `setPos` keeps working) but coalesces to at most one refresh per event-loop turn, and disconnects when the last group is removed. A public `refreshGroupsNow()` gives tests/consumers a synchronous flush.

**Tech Stack:** C++20, Qt 6.8, QTest. Build dir `build/`.
**Prerequisite:** Phase 2 merged — this plan dispatches through `IGraphEdge::endpointsChanged()`.

---

### Task 1: Adjacency index in GraphScene (A4)

**Files:**
- Modify: `src/core/include/graffodil/GraphScene.h` (members, ~line 156; doc comment on addEdge)
- Modify: `src/core/src/GraphScene.cpp` (`addEdge`, `removeEdge`, `removeNode`, `connectedEdges`, `edgeBetween`, `edgeBetweenUndirected`, `adjustEdgesForNode`, `clearGraph`)
- Test: `tests/tst_graphscene.cpp`

- [x] **Step 1: Write the failing/regression tests**

Add to `tests/tst_graphscene.cpp`. The first is the key regression risk: a
naive *directional* index breaks when `GraphEdgeItem::reverse()` swaps
endpoints; keying by both endpoints makes removal direction-agnostic. The
second pins multi-edge and self-loop behavior.

```cpp
void adjacencyIndexSurvivesReverse()
{
    using namespace Graffodil;
    GraphScene scene;
    auto *a = new Test::TestNode("a", {0, 0});
    auto *b = new Test::TestNode("b", {200, 0});
    scene.addNode(a);
    scene.addNode(b);
    auto *edge = new GraphEdgeItem(a, QString(), b, QString(),
                                   std::make_unique<DirectPathStrategy>());
    scene.addEdge(edge);
    const QString id = edge->edgeId();

    edge->reverse();                 // endpoints swap underneath the registry
    scene.removeEdge(id);            // must fully clean up the index

    QVERIFY(scene.connectedEdges("a").isEmpty());
    QVERIFY(scene.connectedEdges("b").isEmpty());
    QVERIFY(scene.edgeForId(id) == nullptr);
    delete edge;
}

void multiEdgesAndSelfLoopIndexedOnce()
{
    using namespace Graffodil;
    GraphScene scene;
    auto *a = new Test::TestNode("a", {0, 0});
    auto *b = new Test::TestNode("b", {200, 0});
    scene.addNode(a);
    scene.addNode(b);
    auto *e1 = new Test::TestEdge("e1", a, b);
    auto *e2 = new Test::TestEdge("e2", a, b);   // parallel edge
    auto *loop = new Test::TestEdge("loop", a, a); // self-loop
    scene.addEdge(e1);
    scene.addEdge(e2);
    scene.addEdge(loop);

    QCOMPARE(scene.connectedEdges("a").size(), 3);  // e1, e2, loop — loop once
    QCOMPARE(scene.connectedEdges("b").size(), 2);

    scene.removeNode("b");                           // cascades e1+e2
    QCOMPARE(scene.connectedEdges("a").size(), 1);   // just the loop
    QCOMPARE(scene.edges().size(), 1);
}
```

(`tst_graphscene.cpp` gains `#include <memory>` if missing; after Phase 2 the
unique_ptr ctor is the non-deprecated one.)

- [x] **Step 2: Run to check current behavior**

Run: `cmake --build build && (cd build && ctest -R tst_graphscene --output-on-failure)`
Expected: PASS already (the linear scans behave correctly) — these tests pin
behavior so the index can't regress it. That's the point: this task is a
performance refactor under test.

- [x] **Step 3: Add the index**

`src/core/include/graffodil/GraphScene.h` — private members, next to
`m_edges`:

```cpp
    /// nodeId -> edges touching that node (both endpoints). An edge with
    /// distinct endpoints appears under two keys; a self-loop under one.
    QMultiHash<QString, IGraphEdge *> m_edgesByNode;
```

On `addEdge`'s doc comment in the header, append:

```cpp
    /// The edge's endpoint *pair* must stay stable while registered
    /// (GraphEdgeItem::reverse() is fine — it only swaps within the pair).
    /// To change endpoints, removeEdge() and re-add.
```

`src/core/src/GraphScene.cpp`:

In `addEdge`, after `m_edges.insert(...)`:

```cpp
    m_edgesByNode.insert(edge->sourceNodeId(), edge);
    if (edge->targetNodeId() != edge->sourceNodeId())
        m_edgesByNode.insert(edge->targetNodeId(), edge);
```

In `removeEdge`, after `IGraphEdge *edge = it.value();`:

```cpp
    // Keyed by both endpoints — removal works regardless of reverse().
    m_edgesByNode.remove(edge->sourceNodeId(), edge);
    m_edgesByNode.remove(edge->targetNodeId(), edge);
```

In `clearGraph`, alongside `m_edges.clear();`:

```cpp
    m_edgesByNode.clear();
```

- [x] **Step 4: Route the lookups through the index**

Replace the four scan methods in `GraphScene.cpp`:

```cpp
QList<IGraphEdge *> GraphScene::connectedEdges(const QString &nodeId) const
{
    return m_edgesByNode.values(nodeId);
}
```

```cpp
IGraphEdge *GraphScene::edgeBetween(const QString &sourceId, const QString &targetId) const
{
    for (auto it = m_edgesByNode.constFind(sourceId);
         it != m_edgesByNode.constEnd() && it.key() == sourceId; ++it) {
        IGraphEdge *edge = it.value();
        if (edge->sourceNodeId() == sourceId && edge->targetNodeId() == targetId)
            return edge;
    }
    return nullptr;
}
```

```cpp
IGraphEdge *GraphScene::edgeBetweenUndirected(const QString &a, const QString &b) const
{
    for (auto it = m_edgesByNode.constFind(a);
         it != m_edgesByNode.constEnd() && it.key() == a; ++it) {
        IGraphEdge *edge = it.value();
        const QString s = edge->sourceNodeId();
        const QString t = edge->targetNodeId();
        if ((s == a && t == b) || (s == b && t == a))
            return edge;
    }
    return nullptr;
}
```

```cpp
void GraphScene::adjustEdgesForNode(const QString &nodeId)
{
    for (auto it = m_edgesByNode.constFind(nodeId);
         it != m_edgesByNode.constEnd() && it.key() == nodeId; ++it)
        it.value()->endpointsChanged();
}
```

And in `removeNode`, replace the manual edge scan (the
`for (auto *edge : m_edges) { ... connectedIds.append ... }` block) with:

```cpp
    QList<QString> connectedIds;
    const QList<IGraphEdge *> touching = m_edgesByNode.values(id);
    connectedIds.reserve(touching.size());
    for (auto *edge : touching)
        connectedIds.append(edge->edgeId());
```

(The follow-up `for (const auto &eid : connectedIds) removeEdge(eid);` stays —
removeEdge mutates the index, so iterate over the snapshot of ids.)

- [x] **Step 5: Run the full suite**

Run: `cmake --build build && (cd build && ctest --output-on-failure)`
Expected: all PASS, including the two new tests and every existing
scene/tools/integration test.

- [x] **Step 6: Commit**

```bash
git add src/core tests/tst_graphscene.cpp
git commit -m "perf(core): adjacency index — per-node edge lookups now O(degree)"
```

---

### Task 2: Coalesced group refresh + disconnect at zero groups (A5)

`refreshGroupsFromSceneChange` currently runs the full groups×members walk on
*every* `changed` emission (hover repaints included). Coalesce to one refresh
per event-loop turn. Synchronous behavior is preserved where consumers rely on
it (`applyLayout`, `addGroup`, `setGroupMembers`, `setGroupStyle` already call
`refreshGroups()` directly) and exposed as `refreshGroupsNow()` for everyone
else.

**Files:**
- Modify: `src/core/include/graffodil/GraphScene.h` (public method + member flag)
- Modify: `src/core/src/GraphScene.cpp` (`refreshGroupsFromSceneChange`, `removeGroup`, `clearGraph`)
- Test: `tests/tst_groups.cpp`

- [x] **Step 1: Write the failing test**

Add to `tests/tst_groups.cpp`:

```cpp
void sceneChangeRefreshIsCoalesced()
{
    using namespace Graffodil;
    GraphScene scene;
    auto *a = new Test::TestNode("a", {0, 0});
    scene.addNode(a);
    const QString gid = scene.addGroup({"a"}, GroupStyle{});
    QCoreApplication::processEvents();   // drain pending refreshes

    const QRectF before = scene.groupItem(gid)->boundingRect();
    a->setPos(500, 500);
    // Programmatic setPos: the refresh must arrive via the event loop —
    // synchronously the bounds are still stale...
    QCoreApplication::processEvents();
    const QRectF after = scene.groupItem(gid)->boundingRect();
    QVERIFY(after != before);

    // ...and a synchronous flush API exists for consumers that need it now:
    a->setPos(900, 900);
    scene.refreshGroupsNow();
    QVERIFY(scene.groupItem(gid)->boundingRect() != after);
}
```

(Match the existing `tst_groups.cpp` style for GroupStyle construction — if
its tests build styles differently, copy that idiom.)

- [x] **Step 2: Run to verify it fails**

Run: `cmake --build build && (cd build && ctest -R tst_groups --output-on-failure)`
Expected: FAIL — `refreshGroupsNow()` doesn't exist yet (compile error).

- [x] **Step 3: Implement**

`src/core/include/graffodil/GraphScene.h` — public, in the group-regions
section:

```cpp
    /// Synchronously recompute all group bounds from current member
    /// positions. Normally unnecessary — refresh is automatic (coalesced to
    /// once per event-loop turn) — but useful right after programmatic
    /// setPos() when bounds are queried before returning to the loop.
    void refreshGroupsNow();
```

Private members, next to `m_refreshingGroups`:

```cpp
    bool m_groupRefreshQueued = false;
```

`src/core/src/GraphScene.cpp`:

```cpp
void GraphScene::refreshGroupsNow()
{
    refreshGroups();
}

void GraphScene::refreshGroupsFromSceneChange(const QList<QRectF> &)
{
    // changed() fires on every item invalidation (hover, selection,
    // repaints) — far more often than members actually move. Coalesce to
    // at most one geometric refresh per event-loop turn.
    if (m_refreshingGroups || m_groupRefreshQueued || m_groups.isEmpty())
        return;
    m_groupRefreshQueued = true;
    QMetaObject::invokeMethod(this, [this] {
        m_groupRefreshQueued = false;
        refreshGroups();
    }, Qt::QueuedConnection);
}
```

In `removeGroup`, after `m_groupMembers.remove(groupId);`:

```cpp
    if (m_groups.isEmpty() && m_groupsChangeConnected) {
        disconnect(this, &QGraphicsScene::changed,
                   this, &GraphScene::refreshGroupsFromSceneChange);
        m_groupsChangeConnected = false;
    }
```

In `clearGraph`, after `m_groupMembers.clear();`, the same disconnect block.
(`addGroup` already reconnects via `Qt::UniqueConnection` + the flag, so
re-adding groups later keeps working.)

- [x] **Step 4: Repair any existing tests that relied on synchronous refresh**

Run: `cmake --build build && (cd build && ctest -R tst_groups --output-on-failure)`

If any pre-existing `tst_groups` case fails, it moved a node via `setPos()` and
asserted bounds immediately. For each failure, insert
`scene.refreshGroupsNow();` (preferred — keeps the test deterministic) between
the move and the assertion. Find candidates with:

```bash
grep -n 'setPos' tests/tst_groups.cpp
```

Do NOT weaken assertions; only add the flush. Cases that go through
`applyLayout()`, `addGroup()`, `setGroupMembers()`, or `setGroupStyle()` are
unaffected (those refresh synchronously).

- [x] **Step 5: Full suite**

Run: `(cd build && ctest --output-on-failure)` — Expected: all PASS.

- [x] **Step 6: Commit**

```bash
git add src/core tests/tst_groups.cpp
git commit -m "perf(core): coalesce group refresh to once per event-loop turn"
```

---

### Task 3: Close out the phase

- [x] Update `docs/graffodil-design.md` §Group regions: note the coalesced
      refresh + `refreshGroupsNow()`; §GraphScene: note the endpoint-stability
      requirement on registered edges.
- [x] Update `docs/ROADMAP.md`: Phase 3 → ☑ with commit hash; Status-log line.
- [x] Sanity-check the demo: drag nodes inside a group at 60fps; group chrome
      follows with no visible lag.
- [x] `git add docs && git commit -m "docs: mark Phase 3 (scene scalability) complete"`
