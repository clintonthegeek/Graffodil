# `AnchorHighlight::boundingRect` dereferences dangling `IGraphNode *m_node`

**Severity:** High — `SIGSEGV` during normal user interaction; happens any
time a node is destroyed while the edge-creation tool still has the
`AnchorHighlight` armed against it. Reliably reproducible under bulk
data churn.
**Reported by:** PlanStan, 2026-05-13 (Stage 5 graph refit dogfood with
a 95-block fixture).
**Component:** `Graffodil::AnchorHighlight`
(`src/core/src/AnchorHighlight.cpp`, `src/core/include/graffodil/AnchorHighlight.h`).
**Triggering caller:** `Graffodil::CreateEdgeTool` (the only consumer of
`AnchorHighlight::showAnchors` / `hideAnchors`).
**Crash surface:** `Graffodil::GraphMinimap::paintEvent` →
`scene()->itemsBoundingRect()` → every scene item's `boundingRect()`.

## Symptom

`SIGSEGV` in `AnchorHighlight::boundingRect` at
`/home/clinton/dev/Graffodil/src/core/src/AnchorHighlight.cpp:44`:

```cpp
QRectF r = m_node->nodeBoundingRect();
```

Full crash backtrace (PlanStan, master @ `282fdd03`, sibling-checkout
consumer of Graffodil HEAD):

```
Thread 1 "PlanStan" received signal SIGSEGV, Segmentation fault.
0x00007ffff7824468 in Graffodil::AnchorHighlight::boundingRect
    (this=0x55555784a0c0) at AnchorHighlight.cpp:44
44          QRectF r = m_node->nodeBoundingRect();
#0  Graffodil::AnchorHighlight::boundingRect ()                AnchorHighlight.cpp:44
#1  QGraphicsItem::sceneBoundingRect ()                        qgraphicsitem.cpp:4784
#2  QGraphicsScene::itemsBoundingRect ()                       qgraphicsscene.cpp:1929
#3  Graffodil::GraphMinimap::paintEvent ()                     GraphMinimap.cpp:46
...
```

PlanStan reproduction: open the dependency graph view in `build-dev/bin/PlanStan`,
import the dogfood fixture (`data/sample-fixtures/dogfood-life.json`, 95 blocks)
via `Developer → Load Fixture…`, then perform any operation that destroys
or re-creates `BlockNode`s — e.g., change a task's status. Crash is
intermittent (depends on whether `CreateEdgeTool` last hovered the now-dead
node) but reproduces within a few status changes when the minimap is visible.

The minimap is the trigger because it's the only paint path that calls
`itemsBoundingRect()` on the whole scene every repaint, which forces a
`boundingRect()` call on the dangling `AnchorHighlight` even when no
edge-creation interaction is in progress. Without the minimap visible,
the same dangling pointer might survive longer (Qt's normal cull skips
items outside the dirty region), but the bug is the same.

## Cause

`AnchorHighlight` (`AnchorHighlight.h:35`) stores a raw non-owning pointer:

```cpp
private:
    IGraphNode *m_node = nullptr;
```

`showAnchors(node)` assigns it; `hideAnchors()` clears it. There is no
notification path from the underlying `IGraphNode` (or its backing
`QGraphicsItem`) back to `AnchorHighlight` when the node is destroyed.
The `CreateEdgeTool` calls `hideAnchors()` only on its own state
transitions — drag end, hover-leave, scene change — none of which fire
when the consumer destroys a node via its model layer (commits a status
edit, recomputes scope, etc.).

Net effect: a hovered-then-destroyed node leaves `AnchorHighlight` with
a dangling `m_node`. The next repaint that touches `boundingRect()` or
`paint()` is UB; on optimized builds it crashes when the deallocated
heap region is unmapped (as in the trace above).

`IGraphNode` is a pure interface (`src/core/include/graffodil/IGraphNode.h`),
not a QObject, so a plain `QPointer<IGraphNode>` won't compile. But in
practice every Graffodil consumer implements `IGraphNode` on a
`QGraphicsObject` subclass (PlanStan's `BlockNode`, Graffodil's own
demo nodes). Implementations therefore *do* have a `QObject` identity
reachable via `IGraphNode::graphicsItem()` → `qobject_cast<QGraphicsObject *>`.

`CreateEdgeTool` itself already uses this defensive pattern for its
reverse direction — `QPointer<AnchorHighlight> m_highlight;`
(`CreateEdgeTool.h:76`). The fix is to apply the same pattern from
`AnchorHighlight` back to its node.

## Recommended fix

Store a `QPointer` guard alongside the raw pointer at `showAnchors` time,
and bail in `boundingRect` / `paint` when the guard is null:

```cpp
// AnchorHighlight.h
#include <QPointer>
class QGraphicsObject;

class AnchorHighlight : public QGraphicsObject {
    // ...
private:
    IGraphNode *m_node = nullptr;
    QPointer<QGraphicsObject> m_nodeGuard;   // null when m_node has been destroyed
    QString m_activeAnchorId;
    bool m_visible = false;
    static constexpr qreal kDotRadius = 5.0;
};
```

```cpp
// AnchorHighlight.cpp
void AnchorHighlight::showAnchors(IGraphNode *node)
{
    if (m_node == node && m_visible)
        return;
    m_node = node;
    m_nodeGuard = node ? qobject_cast<QGraphicsObject *>(node->graphicsItem())
                       : nullptr;
    m_visible = true;
    prepareGeometryChange();
    update();
}

void AnchorHighlight::hideAnchors()
{
    if (!m_visible && m_node == nullptr)
        return;
    m_node = nullptr;
    m_nodeGuard.clear();
    m_visible = false;
    prepareGeometryChange();
    update();
}

QRectF AnchorHighlight::boundingRect() const
{
    if (!m_visible || !m_node || !m_nodeGuard)
        return {};
    QRectF r = m_node->nodeBoundingRect();
    return r.adjusted(-kDotRadius * 2, -kDotRadius * 2,
                      kDotRadius * 2, kDotRadius * 2);
}

void AnchorHighlight::paint(QPainter *painter,
                            const QStyleOptionGraphicsItem *,
                            QWidget *)
{
    if (!m_visible || !m_node || !m_nodeGuard)
        return;
    // ... (rest unchanged)
}
```

After the fix, when the underlying `QGraphicsObject` is destroyed,
`m_nodeGuard` auto-clears and `boundingRect`/`paint` short-circuit to
the empty / no-op path — same as if `hideAnchors()` had been called.
No crash, no visual artifact (the dot for a vanished node was already
incorrect to keep drawing).

### Caveat: non-QGraphicsObject `IGraphNode` implementors

If anyone implements `IGraphNode` on a plain `QGraphicsItem` (not
`QGraphicsObject`), the `qobject_cast` returns null even when the node
is alive — and `m_nodeGuard` is always null — so `showAnchors` becomes
a no-op for that consumer. Two options:

1. **Document the requirement.** Add to `IGraphNode.h`: "Implementations
   that want to be hover-target compatible with `CreateEdgeTool` /
   `AnchorHighlight` must derive from `QGraphicsObject`, not bare
   `QGraphicsItem`." Cheap; matches how every existing consumer
   already works.
2. **Make the requirement enforceable.** Change `IGraphNode::graphicsItem()`'s
   return type to `QGraphicsObject *`. Breaking API change; rejectable
   for a 0.x library, defensible for a 1.x.

Option 1 is the right call for now — Graffodil already documents (in
`AnchorHighlight.h:15–17`) that consumers should use QPointer against
QGraphicsObject items; this just makes the same expectation symmetric.

## Tests

Reasonable unit tests (no event loop required for the first two):

- Construct a node + `AnchorHighlight`, call `showAnchors(node)`,
  `delete node`, then call `highlight.boundingRect()`. Assert it
  returns an empty `QRectF` and does not crash.
- Same setup but with a `QGraphicsScene` and minimap-style call:
  `scene.itemsBoundingRect()` after `delete node`. Assert no crash.
- Sanity: with a live node, `showAnchors(node)` then `boundingRect()`
  returns the adjusted-by-`kDotRadius*2` rect of the node.

## Out of scope for this report

- The minimap is the trigger, not the bug. The minimap correctly asks
  every scene item for its bounding rect; the broken contract is on
  the `AnchorHighlight` side.
- Whether `CreateEdgeTool` should also defensively clear the highlight
  on scene-change events that destroy multiple items (it could, but
  that's a belt-and-braces fix; the QPointer guard is the structural one).

## Downstream notes

PlanStan's `BlockNode` already derives from `QGraphicsObject`, so the
recommended fix lands cleanly with no consumer-side change. Once the
upstream fix ships, PlanStan can remove its current workaround of
hiding the minimap during bulk graph rebuilds (if any — Stage 5 had
not yet adopted such a workaround at the time of this report;
bird-view minimap was simply experienced as crashy and the fixture
load loop became risky to dogfood with the minimap visible).
