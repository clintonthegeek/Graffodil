#include <QTest>
#include <QSignalSpy>
#include <QGraphicsSceneMouseEvent>
#include <graffodil/GraphScene.h>
#include <graffodil/GraphTool.h>
#include <graffodil/SelectMoveTool.h>
#include <graffodil/CreateEdgeTool.h>
#include <graffodil/PanZoomTool.h>
#include <graffodil/CompositeTool.h>
#include <graffodil/DefaultGraphTool.h>
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
};

QTEST_MAIN(TestTools)
#include "tst_tools.moc"
