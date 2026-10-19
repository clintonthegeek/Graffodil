#include <QTest>
#include <QSignalSpy>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QKeyEvent>
#include <graffodil/GraphScene.h>
#include <graffodil/GraphTool.h>
#include <graffodil/SelectMoveTool.h>
#include <graffodil/CreateEdgeTool.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/GraphEdgeItem.h>
#include <graffodil/EdgePathStrategy.h>
#include <graffodil/PanZoomTool.h>
#include <graffodil/CompositeTool.h>
#include <graffodil/DefaultGraphTool.h>
#include <graffodil/IAlignmentStrategy.h>
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

        // Simulate click on node center via QApplication::sendEvent
        QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
        press.setScenePos(QPointF(50, 30));  // center of 100x60 node at (0,0)
        press.setButton(Qt::LeftButton);
        press.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &press);

        QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
        release.setScenePos(QPointF(50, 30));
        release.setButton(Qt::LeftButton);
        release.setButtons(Qt::NoButton);
        QCoreApplication::sendEvent(&scene, &release);

        QCOMPARE(scene.selectedNodes().size(), 1);
    }

    // ASK-M6: a node removed while a consumer still holds the edge must detach
    // the endpoints (no dangling pointer, no ghost path at the dead position).
    void nodeRemovalDetachesConsumerHeldEdge() {
        GraphScene scene;
        auto *a = new TestNode("a", {0, 0});
        auto *b = new TestNode("b", {300, 0});
        scene.addNode(a);
        scene.addNode(b);
        auto *edge = new GraphEdgeItem(a, QString(), b, QString(),
                                       std::make_unique<DirectPathStrategy>());
        scene.addEdge(edge);
        edge->adjust();
        QVERIFY(!edge->path().isEmpty());

        scene.removeNode("a");
        QVERIFY(edge->sourceNode() == nullptr);
        QVERIFY(edge->path().isEmpty());

        // The consumer then frees the node: nothing may dereference it.
        delete a;
        edge->endpointsChanged();
        QVERIFY(edge->path().isEmpty());
        QCOMPARE(edge->sourceNode(), nullptr);
    }

    // ASK-M6: the edge tool resolved its source once and kept the raw pointer;
    // a node destroyed mid-gesture was dereferenced on every mouse move.
    void createEdgeToolSurvivesSourceNodeDestruction() {
        GraphScene scene;
        CreateEdgeTool tool;
        scene.setActiveTool(&tool);

        auto *source = new TestNode("source", {0, 0});
        scene.addNode(source);
        QSignalSpy requested(&tool, &CreateEdgeTool::edgeRequested);

        QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
        press.setScenePos(QPointF(50, 30));
        press.setButton(Qt::LeftButton);
        press.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &press);

        // Host destroys the gesture's source.
        scene.removeNode("source");
        delete source;

        QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
        move.setScenePos(QPointF(200, 30));
        move.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &move);

        QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
        release.setScenePos(QPointF(200, 30));
        release.setButton(Qt::LeftButton);
        release.setButtons(Qt::NoButton);
        QCoreApplication::sendEvent(&scene, &release);

        QCOMPARE(requested.count(), 0);
    }

    // ASK-M6: same class for the drag tool — a dragged node destroyed
    // mid-gesture was dereferenced by the next move and by the final emit.
    void selectMoveToolSurvivesDraggedNodeDestruction() {
        GraphScene scene;
        SelectMoveTool tool;
        scene.setActiveTool(&tool);

        auto *node = new TestNode("n", {0, 0});
        node->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
        scene.addNode(node);

        QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
        press.setScenePos(QPointF(50, 30));
        press.setButton(Qt::LeftButton);
        press.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &press);

        QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
        move.setScenePos(QPointF(90, 30));
        move.setLastScenePos(QPointF(50, 30));
        move.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &move);

        scene.removeNode("n");
        delete node;

        QGraphicsSceneMouseEvent move2(QEvent::GraphicsSceneMouseMove);
        move2.setScenePos(QPointF(140, 30));
        move2.setLastScenePos(QPointF(90, 30));
        move2.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &move2);

        QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
        release.setScenePos(QPointF(140, 30));
        release.setButton(Qt::LeftButton);
        release.setButtons(Qt::NoButton);
        QCoreApplication::sendEvent(&scene, &release);
    }

    void selectToolStatusMessage() {
        SelectMoveTool tool;
        QSignalSpy spy(&tool, &GraphTool::statusMessage);
        GraphScene scene;
        scene.setActiveTool(&tool);
        // Activation should emit status message
        QVERIFY(spy.count() >= 1);
    }

    void createEdgeToolEmitsRequest() {
        GraphScene scene;
        CreateEdgeTool tool;
        scene.setActiveTool(&tool);

        auto *n1 = new TestNode("n1", {0, 0});
        auto *n2 = new TestNode("n2", {200, 0});
        scene.addNode(n1);
        scene.addNode(n2);

        QSignalSpy spy(&tool, &CreateEdgeTool::edgeRequested);

        // Click on source node
        QGraphicsSceneMouseEvent press1(QEvent::GraphicsSceneMousePress);
        press1.setScenePos(QPointF(50, 30));
        press1.setButton(Qt::LeftButton);
        press1.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &press1);

        QGraphicsSceneMouseEvent release1(QEvent::GraphicsSceneMouseRelease);
        release1.setScenePos(QPointF(50, 30));
        release1.setButton(Qt::LeftButton);
        release1.setButtons(Qt::NoButton);
        QCoreApplication::sendEvent(&scene, &release1);

        // Click on target node
        QGraphicsSceneMouseEvent press2(QEvent::GraphicsSceneMousePress);
        press2.setScenePos(QPointF(250, 30));
        press2.setButton(Qt::LeftButton);
        press2.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &press2);

        QGraphicsSceneMouseEvent release2(QEvent::GraphicsSceneMouseRelease);
        release2.setScenePos(QPointF(250, 30));
        release2.setButton(Qt::LeftButton);
        release2.setButtons(Qt::NoButton);
        QCoreApplication::sendEvent(&scene, &release2);

        QCOMPARE(spy.count(), 1);
    }

    void createEdgeToolPreventsSelfLoop() {
        GraphScene scene;
        CreateEdgeTool tool;
        scene.setActiveTool(&tool);

        auto *n1 = new TestNode("n1", {0, 0});
        scene.addNode(n1);

        QSignalSpy spy(&tool, &CreateEdgeTool::edgeRequested);

        // Click on same node twice
        QGraphicsSceneMouseEvent press1(QEvent::GraphicsSceneMousePress);
        press1.setScenePos(QPointF(50, 30));
        press1.setButton(Qt::LeftButton);
        press1.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &press1);

        QGraphicsSceneMouseEvent release1(QEvent::GraphicsSceneMouseRelease);
        release1.setScenePos(QPointF(50, 30));
        release1.setButton(Qt::LeftButton);
        release1.setButtons(Qt::NoButton);
        QCoreApplication::sendEvent(&scene, &release1);

        QGraphicsSceneMouseEvent press2(QEvent::GraphicsSceneMousePress);
        press2.setScenePos(QPointF(50, 30));
        press2.setButton(Qt::LeftButton);
        press2.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &press2);

        QGraphicsSceneMouseEvent release2(QEvent::GraphicsSceneMouseRelease);
        release2.setScenePos(QPointF(50, 30));
        release2.setButton(Qt::LeftButton);
        release2.setButtons(Qt::NoButton);
        QCoreApplication::sendEvent(&scene, &release2);

        QCOMPARE(spy.count(), 0);
    }

    void createEdgeToolDragDropOnEmptyEmitsDropSignal() {
        GraphScene scene;
        CreateEdgeTool tool;
        scene.setActiveTool(&tool);

        auto *n1 = new TestNode("n1", {0, 0});
        scene.addNode(n1);

        // QSignalSpy marshals through QVariant, which needs the raw node
        // pointer type registered; consumers using direct connections don't.
        qRegisterMetaType<IGraphNode *>("IGraphNode*");
        QSignalSpy dropSpy(&tool, &CreateEdgeTool::edgeDroppedOnEmpty);
        QSignalSpy edgeSpy(&tool, &CreateEdgeTool::edgeRequested);

        QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
        press.setScenePos(QPointF(50, 30));
        press.setButton(Qt::LeftButton);
        press.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &press);

        // Drag well past the threshold, out over empty canvas.
        QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
        move.setScenePos(QPointF(400, 400));
        move.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &move);

        QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
        release.setScenePos(QPointF(400, 400));
        release.setButton(Qt::LeftButton);
        release.setButtons(Qt::NoButton);
        QCoreApplication::sendEvent(&scene, &release);

        QCOMPARE(edgeSpy.count(), 0);
        QCOMPARE(dropSpy.count(), 1);
        const QList<QVariant> args = dropSpy.first();
        QCOMPARE(args.at(0).value<IGraphNode *>(), static_cast<IGraphNode *>(n1));
        QCOMPARE(args.at(2).toPointF(), QPointF(400, 400));

        // The tool reset itself: the next press starts a fresh gesture
        // rather than completing the abandoned one.
        QGraphicsSceneMouseEvent press2(QEvent::GraphicsSceneMousePress);
        press2.setScenePos(QPointF(50, 30));
        press2.setButton(Qt::LeftButton);
        press2.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &press2);
        QCOMPARE(edgeSpy.count(), 0);
    }

    void createEdgeToolClickOnEmptyEmitsDropSignal() {
        GraphScene scene;
        CreateEdgeTool tool;
        scene.setActiveTool(&tool);

        auto *n1 = new TestNode("n1", {0, 0});
        scene.addNode(n1);

        QSignalSpy dropSpy(&tool, &CreateEdgeTool::edgeDroppedOnEmpty);

        // Click-click mode: first click picks the source...
        QGraphicsSceneMouseEvent press1(QEvent::GraphicsSceneMousePress);
        press1.setScenePos(QPointF(50, 30));
        press1.setButton(Qt::LeftButton);
        press1.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &press1);

        QGraphicsSceneMouseEvent release1(QEvent::GraphicsSceneMouseRelease);
        release1.setScenePos(QPointF(50, 30));
        release1.setButton(Qt::LeftButton);
        release1.setButtons(Qt::NoButton);
        QCoreApplication::sendEvent(&scene, &release1);

        QCOMPARE(dropSpy.count(), 0);

        // ...second click lands on the background.
        QGraphicsSceneMouseEvent press2(QEvent::GraphicsSceneMousePress);
        press2.setScenePos(QPointF(400, 400));
        press2.setButton(Qt::LeftButton);
        press2.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &press2);

        QCOMPARE(dropSpy.count(), 1);
    }

    void createEdgeToolIdleClickOnEmptyIsSilent() {
        GraphScene scene;
        CreateEdgeTool tool;
        scene.setActiveTool(&tool);

        QSignalSpy dropSpy(&tool, &CreateEdgeTool::edgeDroppedOnEmpty);

        QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
        press.setScenePos(QPointF(400, 400));
        press.setButton(Qt::LeftButton);
        press.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &press);

        QCOMPARE(dropSpy.count(), 0);
    }

    void createEdgeToolSelfLoopDragIsNotADrop() {
        GraphScene scene;
        CreateEdgeTool tool;
        scene.setActiveTool(&tool);

        auto *n1 = new TestNode("n1", {0, 0});
        scene.addNode(n1);

        QSignalSpy dropSpy(&tool, &CreateEdgeTool::edgeDroppedOnEmpty);

        QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
        press.setScenePos(QPointF(10, 10));
        press.setButton(Qt::LeftButton);
        press.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &press);

        QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
        move.setScenePos(QPointF(60, 40));
        move.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &move);

        QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
        release.setScenePos(QPointF(60, 40));
        release.setButton(Qt::LeftButton);
        release.setButtons(Qt::NoButton);
        QCoreApplication::sendEvent(&scene, &release);

        QCOMPARE(dropSpy.count(), 0);
    }

    void panZoomToolActivates() {
        GraphScene scene;
        PanZoomTool tool;
        scene.setActiveTool(&tool);
        QCOMPARE(scene.activeTool(), &tool);
        QCOMPARE(tool.scene(), &scene);
    }

    void compositeRoutesLeftToFirstButMiddleToSecond() {
        GraphScene scene;
        auto *n1 = new TestNode("n1", {0, 0});
        n1->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
        scene.addNode(n1);

        auto *select = new SelectMoveTool;
        auto *panZoom = new PanZoomTool;
        panZoom->setPanButton(Qt::MiddleButton);

        CompositeTool composite;
        composite.addMouseRoute(select, CompositeTool::matchButton(Qt::LeftButton));
        composite.addMouseRoute(panZoom, CompositeTool::matchButton(Qt::MiddleButton));
        scene.setActiveTool(&composite);

        // Left click on node should reach SelectMoveTool and select the node.
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

        QCOMPARE(scene.selectedNodes().size(), 1);

        // Middle-button press should NOT reach SelectMoveTool — so selection
        // is unchanged (still n1) by a middle click elsewhere.
        QGraphicsSceneMouseEvent mpress(QEvent::GraphicsSceneMousePress);
        mpress.setScenePos(QPointF(500, 500));  // empty space
        mpress.setButton(Qt::MiddleButton);
        mpress.setButtons(Qt::MiddleButton);
        QCoreApplication::sendEvent(&scene, &mpress);

        QGraphicsSceneMouseEvent mrelease(QEvent::GraphicsSceneMouseRelease);
        mrelease.setScenePos(QPointF(500, 500));
        mrelease.setButton(Qt::MiddleButton);
        mrelease.setButtons(Qt::NoButton);
        QCoreApplication::sendEvent(&scene, &mrelease);

        // Left click on empty would have cleared selection; middle click
        // didn't touch selection because the SelectMoveTool wasn't reached.
        QCOMPARE(scene.selectedNodes().size(), 1);

        // Clean up sub-tools (not parented)
        delete select;
        delete panZoom;
    }

    void defaultToolConfiguration() {
        DefaultGraphTool tool;
        QVERIFY(tool.selectMoveTool() != nullptr);
        QVERIFY(tool.panZoomTool() != nullptr);
        // PanZoom should be configured for middle-button pan by default.
        QCOMPARE(tool.panZoomTool()->panButton(), Qt::MiddleButton);
        QCOMPARE(tool.panZoomTool()->zoomButton(), Qt::NoButton);
        QCOMPARE(tool.panZoomTool()->zoomWheelModifier(), Qt::NoModifier);
    }

    void defaultToolLeftClickSelectsNode() {
        GraphScene scene;
        auto *n1 = new TestNode("n1", {0, 0});
        n1->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
        scene.addNode(n1);

        DefaultGraphTool tool;
        scene.setActiveTool(&tool);

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

        QCOMPARE(scene.selectedNodes().size(), 1);
    }

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

    void selectToolDeactivateDuringDragEmitsDragEnded() {
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

        QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
        move.setScenePos(QPointF(100, 30));
        move.setLastScenePos(QPointF(50, 30));
        move.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &move);

        QCOMPARE(began.count(), 1);
        QCOMPARE(ended.count(), 0);

        // Switch tools mid-drag — dragEnded must fire even without a release.
        scene.setActiveTool(nullptr);
        QCOMPARE(ended.count(), 1);
    }

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

        // Drag cursor from (50, 30) to (125, 30): free delta is +75, so node
        // pos would be (75, 0). Snap rounds the primary node's x to the
        // nearest 25 -> 75, giving final pos (75, 0).
        QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
        move.setScenePos(QPointF(125, 30));
        move.setLastScenePos(QPointF(50, 30));
        move.setButtons(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &move);

        QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
        release.setScenePos(QPointF(125, 30));
        release.setButton(Qt::LeftButton);
        release.setButtons(Qt::NoButton);
        QCoreApplication::sendEvent(&scene, &release);

        QCOMPARE(n1->graphicsItem()->pos(), QPointF(75, 0));
    }

    void selectToolDeletesSelectionOnDeleteKeyWithoutFocus()
    {
        GraphScene scene;
        SelectMoveTool tool;
        scene.setActiveTool(&tool);

        auto *n1 = new TestNode("n1", {0, 0});
        n1->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
        scene.addNode(n1);
        n1->graphicsItem()->setSelected(true);

        QSignalSpy spy(&tool, &SelectMoveTool::deleteRequested);

        // No scene focus — Delete must still emit deleteRequested, as before.
        QVERIFY(scene.focusItem() == nullptr);
        QKeyEvent ev(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
        QCoreApplication::sendEvent(&scene, &ev);

        QCOMPARE(spy.count(), 1);
    }

    void selectToolBackspaceDeletesSelectionWithoutFocus()
    {
        GraphScene scene;
        SelectMoveTool tool;
        scene.setActiveTool(&tool);

        auto *e1owner = new TestNode("src", {0, 0});
        auto *e1tgt   = new TestNode("tgt", {200, 0});
        scene.addNode(e1owner);
        scene.addNode(e1tgt);
        auto *edge = new TestEdge("e1", e1owner, e1tgt);
        edge->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
        scene.addEdge(edge);
        edge->graphicsItem()->setSelected(true);

        QSignalSpy spy(&tool, &SelectMoveTool::deleteRequested);

        QVERIFY(scene.focusItem() == nullptr);
        QKeyEvent ev(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
        QCoreApplication::sendEvent(&scene, &ev);

        QCOMPARE(spy.count(), 1);
    }

    void selectToolSuppressesKeyboardWhenFocusItemOwnsIt()
    {
        // Focused-editor contract: when an item has scene focus (e.g. a
        // QGraphicsTextItem inside a node card is being edited), Delete
        // must NOT emit deleteRequested, and R must NOT emit
        // reverseRequested — the editor owns those keystrokes.
        GraphScene scene;
        SelectMoveTool tool;
        scene.setActiveTool(&tool);

        auto *n1 = new TestNode("n1", {0, 0});
        n1->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
        scene.addNode(n1);
        n1->graphicsItem()->setSelected(true);

        // Add a sibling QGraphicsTextItem to the scene and give it focus.
        auto *editor = new QGraphicsTextItem;
        editor->setPlainText("editable");
        editor->setTextInteractionFlags(Qt::TextEditorInteraction);
        scene.addItem(editor);
        editor->setFocus();

        QVERIFY(scene.focusItem() == editor);

        QSignalSpy delSpy(&tool, &SelectMoveTool::deleteRequested);
        QSignalSpy revSpy(&tool, &SelectMoveTool::reverseRequested);

        QKeyEvent del(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
        QCoreApplication::sendEvent(&scene, &del);

        QKeyEvent bs(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
        QCoreApplication::sendEvent(&scene, &bs);

        QKeyEvent r(QEvent::KeyPress, Qt::Key_R, Qt::NoModifier);
        QCoreApplication::sendEvent(&scene, &r);

        QCOMPARE(delSpy.count(), 0);
        QCOMPARE(revSpy.count(), 0);

        // Selected node must still be selected — the tool did not see the keys.
        QVERIFY(n1->graphicsItem()->isSelected());
    }

    void selectToolSuppressesKeyboardWhenFocusItemOwnsItDirect()
    {
        // Same contract, but driving the tool directly (not through
        // GraphScene). This pins down the in-tool guard's purpose — it
        // keeps the tool correct even when consumed outside the scene.
        GraphScene scene;
        SelectMoveTool tool;
        scene.setActiveTool(&tool);

        auto *n1 = new TestNode("n1", {0, 0});
        n1->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
        scene.addNode(n1);
        n1->graphicsItem()->setSelected(true);

        auto *editor = new QGraphicsTextItem;
        editor->setPlainText("editable");
        editor->setTextInteractionFlags(Qt::TextEditorInteraction);
        scene.addItem(editor);
        editor->setFocus();

        QSignalSpy delSpy(&tool, &SelectMoveTool::deleteRequested);

        QKeyEvent del(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
        tool.keyPressEvent(&del);

        QCOMPARE(delSpy.count(), 0);
    }
};

QTEST_MAIN(TestTools)
#include "tst_tools.moc"
