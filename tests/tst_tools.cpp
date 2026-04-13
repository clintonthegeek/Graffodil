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
};

QTEST_MAIN(TestTools)
#include "tst_tools.moc"
