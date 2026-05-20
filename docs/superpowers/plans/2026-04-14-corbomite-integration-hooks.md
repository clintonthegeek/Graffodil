# Corbomite Integration Hooks — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Land the three "Yes — upstream" items from `docs/corbomite-integration-feedback.md` so Corbomite can migrate Canvas onto Graffodil without bespoke forks: a cache-compositing hook for rich-content nodes in Batch mode, explicit drag boundary signals plus a signal catalog for undo integration, and a minimal alignment-strategy extension point on `SelectMoveTool`.

**Architecture:**
- A new header-only interface `IRenderableCache` lets nodes advertise a pre-rendered pixmap; `BatchRenderer::synchronize()` detects it via `dynamic_cast` and feeds the pixmap into `BatchNodeItem`, which composites it in `paint()` in place of the default ellipse. Non-cache nodes are unchanged.
- `SelectMoveTool` gains `dragBegan` / `dragEnded` signals (per-user-action boundaries) alongside the existing `nodeMoved`. A new `IAlignmentStrategy` interface lets consumers filter the live drag position and paint guide overlays — Core ships no default implementation.
- Signal catalog and cache contract are documented in `docs/graffodil-design.md`.

**Tech Stack:** C++17, Qt6 (`Widgets`, `Test`), CMake, ctest. All new code is in `Graffodil::Core` and `Graffodil::Batch`. Tests use the existing QTest pattern in `tests/`.

---

## File Structure

### New files

- `src/core/include/graffodil/IRenderableCache.h` — cache-compositing contract (interface only, no .cpp)
- `src/core/include/graffodil/IAlignmentStrategy.h` — alignment filter contract (interface only, no .cpp)

### Modified files

- `src/core/include/graffodil/SelectMoveTool.h` — add `dragBegan` / `dragEnded` signals, alignment-strategy setter
- `src/core/src/SelectMoveTool.cpp` — emit the new signals, consult alignment strategy in `mouseMoveEvent`, paint guides
- `src/core/CMakeLists.txt` — install the two new headers
- `src/batch/src/BatchNodeItem.h` — add optional `QPixmap cachedPixmap` + `QSize cachedSize` to `NodeData`
- `src/batch/src/BatchNodeItem.cpp` — draw the pixmap (if present) centered on `position` instead of the ellipse
- `src/batch/src/BatchRenderer.cpp` — `dynamic_cast` each node to `IRenderableCache*`, pull pixmap + size into `NodeData`
- `docs/graffodil-design.md` — new "Mutation signals" subsection, new "Rich-content node caching" subsection, alignment-strategy mention under §Provided Tools
- `tests/tst_tools.cpp` — tests for `dragBegan` / `dragEnded` ordering, alignment strategy snapping
- `tests/tst_batchrenderer.cpp` — test that a cache-providing node produces a batch paint that samples the pixmap

Each file has one responsibility; the two new interfaces are pure virtual and contain no implementation.

---

## Task 1: IRenderableCache interface

**Files:**
- Create: `src/core/include/graffodil/IRenderableCache.h`
- Modify: `src/core/CMakeLists.txt`

- [ ] **Step 1.1: Write the header**

Create `src/core/include/graffodil/IRenderableCache.h` with this exact content:

```cpp
#pragma once

#include <QPixmap>
#include <QSize>

namespace Graffodil {

/// Optional capability interface for IGraphNode implementers whose paint()
/// is expensive (markdown, thumbnails, composed glyphs, etc.).
///
/// Batch mode (Graffodil::Batch) queries every node via dynamic_cast to
/// IRenderableCache*. When a node provides one, BatchNodeItem composites
/// the returned pixmap instead of drawing its default ellipse. Nodes that
/// do not implement this interface are drawn with the default primitive —
/// there is no behaviour change for existing consumers.
///
/// Implementers are responsible for caching. The Batch layer calls
/// cachedPixmap() each time it synchronises; implementations should
/// return a stored pixmap and regenerate only when content changes,
/// invalidating via invalidateCache() from the consumer side.
class IRenderableCache {
public:
    virtual ~IRenderableCache() = default;

    /// Return a pixmap representation sized for `requestedSize`.
    /// Implementations may return a pixmap of any size; the Batch layer
    /// scales to `contentSize()` at paint time.
    virtual QPixmap cachedPixmap(const QSize &requestedSize) = 0;

    /// Logical size the pixmap should be drawn at, in scene units.
    /// Distinct from the node's boundingRect when the node paints with
    /// padding, margins, or scaling.
    virtual QSize contentSize() const = 0;

    /// Cache-invalidation hook. Consumers call this when the underlying
    /// content changes (e.g. markdown text edited, thumbnail regenerated).
    /// Implementations typically clear their stored QPixmap so the next
    /// cachedPixmap() call rebuilds it.
    virtual void invalidateCache() = 0;
};

} // namespace Graffodil
```

- [ ] **Step 1.2: Register the header in CMake**

Edit `src/core/CMakeLists.txt`. Inside the `target_sources` block, after the `include/graffodil/IGraphNode.h` line, add:

```cmake
    include/graffodil/IRenderableCache.h
```

- [ ] **Step 1.3: Build to verify the header compiles**

Run: `cmake --build build --target graffodil-core`
Expected: succeeds with no warnings. If `build/` is missing or stale, first run `cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`.

- [ ] **Step 1.4: Commit**

```bash
git add src/core/include/graffodil/IRenderableCache.h src/core/CMakeLists.txt
git commit -m "feat(core): IRenderableCache capability interface for Batch compositing"
```

---

## Task 2: BatchNodeItem supports cached pixmaps

**Files:**
- Modify: `src/batch/src/BatchNodeItem.h`
- Modify: `src/batch/src/BatchNodeItem.cpp`
- Modify: `tests/tst_batchrenderer.cpp`

- [ ] **Step 2.1: Write the failing test**

Append to `tests/tst_batchrenderer.cpp`, inside the `TestBatchRenderer` class, a new private slot. The test uses a `TestNode` subclass that also implements `IRenderableCache`, runs the renderer through `setBatchMode(true)`, and paints it into a `QImage` — pixel-sampling the image at the node's centre verifies the pixmap (coloured red) was used rather than the default blue ellipse.

Add these includes to the top of the file (below existing includes):

```cpp
#include <graffodil/IRenderableCache.h>
#include <QImage>
#include <QPainter>
```

Add this helper class inside the anonymous namespace or at file scope (outside `TestBatchRenderer`):

```cpp
class CachedNode : public TestNode, public Graffodil::IRenderableCache {
public:
    CachedNode(const QString &id, const QPointF &pos)
        : TestNode(id, pos)
    {
        m_pix = QPixmap(50, 30);
        m_pix.fill(Qt::red);
    }
    QPixmap cachedPixmap(const QSize &) override { return m_pix; }
    QSize contentSize() const override { return QSize(50, 30); }
    void invalidateCache() override { m_pix = {}; }
private:
    QPixmap m_pix;
};
```

Add this test slot inside `TestBatchRenderer`:

```cpp
void batchCompositesCachedPixmap() {
    GraphScene scene;
    auto *n1 = new CachedNode("n1", {100, 100});
    scene.addNode(n1);

    BatchRenderer renderer(&scene);
    renderer.setBatchMode(true);
    renderer.synchronize();

    // Paint the scene into an image and sample the pixel at the node centre.
    QImage img(300, 300, QImage::Format_ARGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    scene.render(&p);
    p.end();

    const QRgb centre = img.pixel(100, 100);
    QCOMPARE(qRed(centre), 255);
    QVERIFY(qGreen(centre) < 50);
    QVERIFY(qBlue(centre) < 50);
}
```

- [ ] **Step 2.2: Run the test and verify it fails**

Run: `cmake --build build --target tst_batchrenderer && ctest --test-dir build -R tst_batchrenderer --output-on-failure`
Expected: compiles (once the new slot references types that already exist) but fails at the `QCOMPARE(qRed(centre), 255)` line because `BatchNodeItem` currently paints a blue ellipse regardless of the node type.

- [ ] **Step 2.3: Extend NodeData with a pixmap**

Edit `src/batch/src/BatchNodeItem.h`. Inside `struct NodeData`, after the `QColor color` line, add two fields:

```cpp
        QPixmap cachedPixmap;   // empty if the node has no IRenderableCache
        QSizeF cachedSize;      // logical size to draw cachedPixmap at
```

Add `#include <QPixmap>` and `#include <QSizeF>` near the other includes at the top of the file.

- [ ] **Step 2.4: Paint the pixmap when present**

Edit `src/batch/src/BatchNodeItem.cpp`. Replace the inner loop of `paint()` (the `for (const auto &node : m_nodes)` block) with:

```cpp
    for (const auto &node : m_nodes) {
        if (!node.cachedPixmap.isNull() && node.cachedSize.isValid()) {
            const QRectF target(node.position.x() - node.cachedSize.width() / 2.0,
                                node.position.y() - node.cachedSize.height() / 2.0,
                                node.cachedSize.width(),
                                node.cachedSize.height());
            painter->drawPixmap(target, node.cachedPixmap, node.cachedPixmap.rect());
            continue;
        }
        painter->setBrush(node.color);
        if (lod < 0.3)
            painter->fillRect(QRectF(node.position.x() - node.radius, node.position.y() - node.radius,
                                     node.radius * 2, node.radius * 2), painter->brush());
        else
            painter->drawEllipse(node.position, node.radius, node.radius);
    }
```

The existing `painter->setPen(Qt::NoPen);` line above the loop stays.

- [ ] **Step 2.5: Populate the pixmap in BatchRenderer**

Edit `src/batch/src/BatchRenderer.cpp`. Add `#include <graffodil/IRenderableCache.h>` below the existing `#include <graffodil/IGraphNode.h>`.

In `synchronize()`, inside the node-building loop (after `nd.color = QColor(100, 150, 200);` and before `nodeData.append(nd);`), add:

```cpp
        if (auto *cache = dynamic_cast<IRenderableCache *>(node)) {
            const QSize sz = cache->contentSize();
            if (sz.isValid()) {
                nd.cachedPixmap = cache->cachedPixmap(sz);
                nd.cachedSize = QSizeF(sz);
            }
        }
```

- [ ] **Step 2.6: Run the test to verify it passes**

Run: `cmake --build build --target tst_batchrenderer && ctest --test-dir build -R tst_batchrenderer --output-on-failure`
Expected: all batch tests pass, including `batchCompositesCachedPixmap`.

- [ ] **Step 2.7: Run the full suite to confirm no regression**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`
Expected: all tests pass.

- [ ] **Step 2.8: Commit**

```bash
git add src/batch/src/BatchNodeItem.h src/batch/src/BatchNodeItem.cpp \
        src/batch/src/BatchRenderer.cpp tests/tst_batchrenderer.cpp
git commit -m "feat(batch): composite IRenderableCache pixmaps in BatchNodeItem"
```

---

## Task 3: SelectMoveTool drag boundary signals

**Files:**
- Modify: `src/core/include/graffodil/SelectMoveTool.h`
- Modify: `src/core/src/SelectMoveTool.cpp`
- Modify: `tests/tst_tools.cpp`

- [ ] **Step 3.1: Write the failing test**

Append this slot to `TestTools` in `tests/tst_tools.cpp`:

```cpp
void selectToolEmitsDragBoundarySignals() {
    GraphScene scene;
    SelectMoveTool tool;
    scene.setActiveTool(&tool);

    auto *n1 = new TestNode("n1", {0, 0});
    n1->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
    scene.addNode(n1);

    QSignalSpy began(&tool, &SelectMoveTool::dragBegan);
    QSignalSpy ended(&tool, &SelectMoveTool::dragEnded);
    QSignalSpy moved(&tool, &SelectMoveTool::nodeMoved);

    // Press on node centre.
    QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
    press.setScenePos(QPointF(50, 30));
    press.setButton(Qt::LeftButton);
    press.setButtons(Qt::LeftButton);
    QCoreApplication::sendEvent(&scene, &press);
    QCOMPARE(began.count(), 0);  // drag not yet started

    // Move past the 3px threshold — this is when the drag truly begins.
    QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
    move.setScenePos(QPointF(100, 30));
    move.setLastScenePos(QPointF(50, 30));
    move.setButtons(Qt::LeftButton);
    QCoreApplication::sendEvent(&scene, &move);
    QCOMPARE(began.count(), 1);

    // Release — nodeMoved, then dragEnded, in that order.
    QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
    release.setScenePos(QPointF(100, 30));
    release.setButton(Qt::LeftButton);
    release.setButtons(Qt::NoButton);
    QCoreApplication::sendEvent(&scene, &release);

    QCOMPARE(moved.count(), 1);
    QCOMPARE(ended.count(), 1);
}

void selectToolSkipsDragSignalsOnClickWithoutMove() {
    GraphScene scene;
    SelectMoveTool tool;
    scene.setActiveTool(&tool);

    auto *n1 = new TestNode("n1", {0, 0});
    n1->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
    scene.addNode(n1);

    QSignalSpy began(&tool, &SelectMoveTool::dragBegan);
    QSignalSpy ended(&tool, &SelectMoveTool::dragEnded);

    QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
    press.setScenePos(QPointF(50, 30));
    press.setButton(Qt::LeftButton);
    press.setButtons(Qt::LeftButton);
    QCoreApplication::sendEvent(&scene, &press);

    QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
    release.setScenePos(QPointF(50, 30));
    release.setButton(Qt::LeftButton);
    release.setButtons(Qt::NoButton);
    QCoreApplication::sendEvent(&scene, &release);

    QCOMPARE(began.count(), 0);
    QCOMPARE(ended.count(), 0);
}
```

- [ ] **Step 3.2: Run the test to verify it fails**

Run: `cmake --build build --target tst_tools`
Expected: compile error — `dragBegan` and `dragEnded` are not members of `SelectMoveTool`.

- [ ] **Step 3.3: Declare the signals**

Edit `src/core/include/graffodil/SelectMoveTool.h`. In the `signals:` section, after the existing `reverseRequested` declaration, add:

```cpp
    /// Emitted once when a drag of one or more nodes actually begins
    /// (i.e. the cursor crosses the drag threshold after a press on a
    /// node). Consumers typically use this to open an undo macro/compound.
    void dragBegan(const QList<IGraphNode *> &nodes);

    /// Emitted once after the final `nodeMoved` of a drag, on mouse
    /// release. Consumers typically use this to close the undo macro
    /// opened in dragBegan. Not emitted if the drag never started
    /// (click without crossing the threshold).
    void dragEnded(const QList<IGraphNode *> &nodes);
```

- [ ] **Step 3.4: Emit dragBegan on threshold crossing**

Edit `src/core/src/SelectMoveTool.cpp`. In `mouseMoveEvent`, replace the node-drag block (starting at `// Node drag` through `m_scene->adjustEdgesForNode(n->nodeId());` / end of loop) with:

```cpp
    // Node drag
    if (m_dragNode && !m_draggedNodes.isEmpty()) {
        QPointF delta = event->scenePos() - event->lastScenePos();
        if (!m_dragging && (event->scenePos() - m_pressScenePos).manhattanLength() < 3)
            return;  // small threshold to avoid tiny accidental moves
        const bool wasDragging = m_dragging;
        m_dragging = true;
        if (!wasDragging)
            emit dragBegan(m_draggedNodes);
        for (auto *n : m_draggedNodes) {
            n->graphicsItem()->setPos(n->graphicsItem()->pos() + delta);
            m_scene->adjustEdgesForNode(n->nodeId());
        }
    }
```

- [ ] **Step 3.5: Emit dragEnded on release**

In `mouseReleaseEvent`, replace the `// Finalize drag` block (from `if (m_dragNode && m_dragging)` through the closing `}`) with:

```cpp
    // Finalize drag
    if (m_dragNode && m_dragging) {
        for (auto *n : m_draggedNodes) {
            QPointF oldPos = m_dragStartPositions.value(n);
            QPointF newPos = n->graphicsItem()->pos();
            if (oldPos != newPos)
                emit nodeMoved(n, oldPos, newPos);
        }
        emit dragEnded(m_draggedNodes);
    }
```

The four trailing reset lines (`m_dragNode = nullptr;`, `m_draggedNodes.clear();`, `m_dragStartPositions.clear();`, `m_dragging = false;`) stay unchanged after that block.

- [ ] **Step 3.6: Run the tests to verify they pass**

Run: `cmake --build build --target tst_tools && ctest --test-dir build -R tst_tools --output-on-failure`
Expected: `selectToolEmitsDragBoundarySignals` and `selectToolSkipsDragSignalsOnClickWithoutMove` both pass, no existing test regresses.

- [ ] **Step 3.7: Commit**

```bash
git add src/core/include/graffodil/SelectMoveTool.h \
        src/core/src/SelectMoveTool.cpp tests/tst_tools.cpp
git commit -m "feat(core): dragBegan/dragEnded signals on SelectMoveTool"
```

---

## Task 4: IAlignmentStrategy extension point

**Files:**
- Create: `src/core/include/graffodil/IAlignmentStrategy.h`
- Modify: `src/core/CMakeLists.txt`
- Modify: `src/core/include/graffodil/SelectMoveTool.h`
- Modify: `src/core/src/SelectMoveTool.cpp`
- Modify: `tests/tst_tools.cpp`

- [ ] **Step 4.1: Write the interface header**

Create `src/core/include/graffodil/IAlignmentStrategy.h`:

```cpp
#pragma once

#include <QPointF>
#include <QList>

namespace Graffodil {

class IGraphNode;

/// Pluggable alignment-guide strategy used by SelectMoveTool during node
/// drags.
///
/// Consumers implement this to get pixel-snap behaviour (align to other
/// nodes, align to a grid, Canvas-style ruler guides, etc.). Core ships
/// no default implementation — with no strategy installed, SelectMoveTool
/// behaves exactly as before (free-drag, no snapping, no guides).
///
/// The strategy is consulted per mouseMove during an active drag. It
/// receives the *proposed* scene position of the primary drag node
/// (`m_dragNode` — the node under the cursor at press time) and may
/// return an adjusted position plus zero or more guide lines for the
/// tool to paint as overlays.
class IAlignmentStrategy {
public:
    virtual ~IAlignmentStrategy() = default;

    struct Result {
        /// Adjusted position for the primary drag node. Return the input
        /// unchanged to decline snapping on this frame.
        QPointF position;
        /// Guide lines to paint (scene coordinates). Empty = no overlay.
        QList<QLineF> guides;
    };

    /// Called each mouseMove during an active drag.
    /// `proposed` is the free-drag position the tool would apply with no
    /// strategy installed. `primary` is the node under the cursor; all
    /// other selected nodes will be translated by the same delta that
    /// the strategy applies to `primary`.
    /// `allNodes` is every node currently in the scene — the strategy
    /// can pick alignment targets from it.
    virtual Result align(IGraphNode *primary,
                         const QPointF &proposed,
                         const QList<IGraphNode *> &allNodes) = 0;
};

} // namespace Graffodil
```

- [ ] **Step 4.2: Register the new header**

Edit `src/core/CMakeLists.txt`. In the `target_sources` block, after the `include/graffodil/IRenderableCache.h` line from Task 1, add:

```cmake
    include/graffodil/IAlignmentStrategy.h
```

- [ ] **Step 4.3: Write the failing test**

Append to `tests/tst_tools.cpp`:

```cpp
void selectToolAppliesAlignmentStrategy() {
    GraphScene scene;
    SelectMoveTool tool;
    scene.setActiveTool(&tool);

    auto *n1 = new TestNode("n1", {0, 0});
    n1->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
    scene.addNode(n1);

    // Strategy: snap x coordinate to multiples of 25.
    struct SnapX : public IAlignmentStrategy {
        Result align(IGraphNode *, const QPointF &proposed,
                     const QList<IGraphNode *> &) override {
            Result r;
            r.position = QPointF(qRound(proposed.x() / 25.0) * 25.0, proposed.y());
            return r;
        }
    };
    SnapX snap;
    tool.setAlignmentStrategy(&snap);

    QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
    press.setScenePos(QPointF(50, 30));
    press.setButton(Qt::LeftButton);
    press.setButtons(Qt::LeftButton);
    QCoreApplication::sendEvent(&scene, &press);

    // Drag cursor from (50, 30) to (112, 30): free delta is +62, so node
    // pos would be (62, 0). Snap rounds the primary node's x to the
    // nearest 25 → 75, giving final pos (75, 0).
    QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
    move.setScenePos(QPointF(112, 30));
    move.setLastScenePos(QPointF(50, 30));
    move.setButtons(Qt::LeftButton);
    QCoreApplication::sendEvent(&scene, &move);

    QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
    release.setScenePos(QPointF(112, 30));
    release.setButton(Qt::LeftButton);
    release.setButtons(Qt::NoButton);
    QCoreApplication::sendEvent(&scene, &release);

    QCOMPARE(n1->graphicsItem()->pos(), QPointF(75, 0));
}
```

Also add `#include <graffodil/IAlignmentStrategy.h>` to the includes at the top of `tst_tools.cpp`.

- [ ] **Step 4.4: Declare the setter and internals**

Edit `src/core/include/graffodil/SelectMoveTool.h`. Add a forward declaration below `class IGraphEdge;`:

```cpp
class IAlignmentStrategy;
```

In the `public:` section, after the existing key/mouse overrides, add:

```cpp
    /// Install an alignment strategy consulted during drags. The tool
    /// does not take ownership. Pass nullptr to disable (default).
    void setAlignmentStrategy(IAlignmentStrategy *strategy);
    IAlignmentStrategy *alignmentStrategy() const { return m_alignment; }
```

In the `private:` section, after `bool m_dragging = false;`, add:

```cpp
    IAlignmentStrategy *m_alignment = nullptr;
    QGraphicsItemGroup *m_guideOverlay = nullptr;
```

And add forward declaration at file top (outside the namespace block, near the existing `class QGraphicsRectItem;`):

```cpp
class QGraphicsItemGroup;
```

- [ ] **Step 4.5: Consume the strategy during drag**

Edit `src/core/src/SelectMoveTool.cpp`. Add to the top include block:

```cpp
#include <graffodil/IAlignmentStrategy.h>
#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
```

Add the setter implementation after `SelectMoveTool::deactivate()`:

```cpp
void SelectMoveTool::setAlignmentStrategy(IAlignmentStrategy *strategy)
{
    m_alignment = strategy;
}
```

In `deactivate()`, before the closing `GraphTool::deactivate();`, add guide cleanup:

```cpp
    if (m_guideOverlay && m_scene) {
        m_scene->removeItem(m_guideOverlay);
        delete m_guideOverlay;
        m_guideOverlay = nullptr;
    }
```

Replace the node-drag block in `mouseMoveEvent` (the one you rewrote in Step 3.4) with this version that routes through the strategy and maintains a guide overlay:

```cpp
    // Node drag
    if (m_dragNode && !m_draggedNodes.isEmpty()) {
        if (!m_dragging && (event->scenePos() - m_pressScenePos).manhattanLength() < 3)
            return;  // small threshold to avoid tiny accidental moves
        const bool wasDragging = m_dragging;
        m_dragging = true;
        if (!wasDragging)
            emit dragBegan(m_draggedNodes);

        const QPointF proposed = m_dragNode->graphicsItem()->pos()
                                 + (event->scenePos() - event->lastScenePos());
        QPointF applied = proposed;
        QList<QLineF> guides;
        if (m_alignment) {
            auto result = m_alignment->align(m_dragNode, proposed, m_scene->nodes());
            applied = result.position;
            guides = result.guides;
        }
        const QPointF delta = applied - m_dragNode->graphicsItem()->pos();
        for (auto *n : m_draggedNodes) {
            n->graphicsItem()->setPos(n->graphicsItem()->pos() + delta);
            m_scene->adjustEdgesForNode(n->nodeId());
        }

        // Rebuild guide overlay.
        if (m_guideOverlay) {
            m_scene->removeItem(m_guideOverlay);
            delete m_guideOverlay;
            m_guideOverlay = nullptr;
        }
        if (!guides.isEmpty()) {
            m_guideOverlay = new QGraphicsItemGroup;
            m_guideOverlay->setZValue(9999);
            for (const QLineF &g : guides) {
                auto *line = new QGraphicsLineItem(g, m_guideOverlay);
                line->setPen(QPen(QColor(255, 100, 200), 0.0, Qt::DashLine));
            }
            m_scene->addItem(m_guideOverlay);
        }
    }
```

In `mouseReleaseEvent`, inside the `// Finalize drag` block (just before `emit dragEnded(m_draggedNodes);`), insert guide cleanup:

```cpp
        if (m_guideOverlay) {
            m_scene->removeItem(m_guideOverlay);
            delete m_guideOverlay;
            m_guideOverlay = nullptr;
        }
```

- [ ] **Step 4.6: Run the tests**

Run: `cmake --build build --target tst_tools && ctest --test-dir build -R tst_tools --output-on-failure`
Expected: `selectToolAppliesAlignmentStrategy` passes. Existing drag tests (including the boundary-signal test from Task 3) still pass.

- [ ] **Step 4.7: Run the full suite**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`
Expected: all tests pass.

- [ ] **Step 4.8: Commit**

```bash
git add src/core/include/graffodil/IAlignmentStrategy.h \
        src/core/include/graffodil/SelectMoveTool.h \
        src/core/src/SelectMoveTool.cpp \
        src/core/CMakeLists.txt \
        tests/tst_tools.cpp
git commit -m "feat(core): IAlignmentStrategy extension point for SelectMoveTool"
```

---

## Task 5: Document the signal contract and new hooks

**Files:**
- Modify: `docs/graffodil-design.md`
- Modify: `docs/corbomite-integration-feedback.md` (append a short status note)

- [ ] **Step 5.1: Add the Mutation Signals section to the design doc**

Edit `docs/graffodil-design.md`. After the `### Provided Tools` section (ends just before `## Layout Engines`), insert a new subsection:

````markdown
### Mutation signal catalog

Graffodil tools never mutate the data model directly — they emit signals and the consumer decides. This catalog lists every mutation-relevant signal each provided tool emits, so consumers wiring an undo stack have a complete picture.

**`SelectMoveTool`**

| Signal | When | Typical undo action |
|---|---|---|
| `dragBegan(nodes)` | Cursor crosses the 3-unit threshold after press-on-node; fires once per drag | Open an undo macro / compound command |
| `nodeMoved(node, oldPos, newPos)` | On mouse release, once per moved node in the drag set, only if the position actually changed | Push a move command into the open macro |
| `dragEnded(nodes)` | On mouse release, after all `nodeMoved` signals for the drag | Close the undo macro |
| `deleteRequested(nodes, edges)` | Delete / Backspace pressed while items are selected | Delete the items; push a delete command |
| `reverseRequested(edges)` | R pressed while edges are selected | Call `GraphEdgeItem::reverse()`; push a reverse command |

Click-without-drag does not emit `dragBegan` / `dragEnded`. Multi-select drag emits one `dragBegan`, one `nodeMoved` per moved node, and one `dragEnded` — so a consumer gets one undo macro containing N move commands per user drag.

**`CreateEdgeTool`**

| Signal | When | Typical undo action |
|---|---|---|
| `edgeRequested(source, sourceAnchorId, target, targetAnchorId)` | User completes a click-click or drag gesture between two distinct nodes | Construct the edge, add it to scene + model, push a create command |

**`GraphScene`**

| Signal | When |
|---|---|
| `nodeAdded(node)` / `nodeRemoved(id)` | `addNode` / `removeNode` called (structural — not user-intent) |
| `edgeAdded(edge)` / `edgeRemoved(id)` | `addEdge` / `removeEdge` called (structural — not user-intent) |

Scene signals fire from the registry calls themselves, so they do not distinguish consumer-driven additions from tool-driven ones. For undo capture, connect to tool signals, not scene signals.

**Not currently emitted (by design)**

- *Edge redirect* (dragging an existing edge's endpoint to a different node or anchor). No provided tool performs this yet; consumers implementing it should emit their own signal or use `GraphEdgeItem::setSourceAnchorId` / `setTargetAnchorId` through their own tool subclass.
- *Tool-activation-as-undo-unit*. Tool switching is UI state, not data mutation — no signal.
````

- [ ] **Step 5.2: Add the cache-compositing section**

In `docs/graffodil-design.md`, after the `## Batch Rendering Module` section's code block (ends just before `## Utility Widgets (in Core)`), insert:

````markdown
### Rich-content node caching

Nodes whose `paint()` is expensive (markdown, thumbnails, composed glyphs) can opt into batch-mode compositing by additionally implementing `IRenderableCache`:

```cpp
class MyCardNode : public QGraphicsItem,
                   public Graffodil::IGraphNode,
                   public Graffodil::IRenderableCache {
public:
    QPixmap cachedPixmap(const QSize &) override { return m_cache; }
    QSize contentSize() const override { return m_logicalSize; }
    void invalidateCache() override { m_cache = {}; }
    // ... IGraphNode + paint() as usual
private:
    QPixmap m_cache;   // rebuilt when content changes
    QSize m_logicalSize;
};
```

When Batch mode is active, `BatchRenderer::synchronize()` queries every node via `dynamic_cast<IRenderableCache*>` and composites the returned pixmap in place of the default ellipse. Nodes that don't implement the interface fall through to the default path unchanged — this is purely additive.
````

- [ ] **Step 5.3: Add the alignment strategy mention**

In `docs/graffodil-design.md`, inside the `**SelectMoveTool**` bullet list under `### Provided Tools`, replace the line `- Drag on node: move (with optional alignment snap guides)` with:

```markdown
- Drag on node: move. Optional alignment via `IAlignmentStrategy` — consumers install a strategy via `setAlignmentStrategy()` to get snap behaviour and paint guide overlays; no default is shipped (consumers own the alignment policy).
```

- [ ] **Step 5.4: Record the landing status in the feedback doc**

Edit `docs/corbomite-integration-feedback.md`. At the very end of the file (after the last line), append:

```markdown

---

## Implementation status (2026-04-14)

- **`IRenderableCache`** — landed in `Graffodil::Core` (`IRenderableCache.h`). Batch mode composites the cached pixmap automatically.
- **Undo signal contract** — `dragBegan` / `dragEnded` added to `SelectMoveTool`; full signal catalog documented in `graffodil-design.md` §Mutation signal catalog.
- **Alignment snap-guide extensibility** — `IAlignmentStrategy` interface added; `SelectMoveTool::setAlignmentStrategy()` lets consumers filter the live drag position and supply guide lines. Core ships no default implementation.
- **Qt Quick fallback** — deferred; no API changes required.
```

- [ ] **Step 5.5: Commit**

```bash
git add docs/graffodil-design.md docs/corbomite-integration-feedback.md
git commit -m "docs: mutation signal catalog, IRenderableCache, alignment strategy"
```

---

## Self-Review Summary

- **Spec coverage:** Item 1 (`IRenderableCache`) → Tasks 1–2. Item 2 (undo signals + audit) → Task 3 + Task 5.1. Item 3 (alignment extensibility) → Task 4 + Task 5.3. Item 4 (Qt Quick) deferred, noted in Task 5.4.
- **Placeholder scan:** No TBDs; every step includes the exact code or command.
- **Type consistency:** `IAlignmentStrategy::Result` used consistently; `QGraphicsItemGroup *m_guideOverlay` member named the same way in header declaration and .cpp use; signal names `dragBegan` / `dragEnded` match between header, implementation, tests, and documentation.

---

Plan complete.
