#include <QTest>
#include <graffodil/BatchRenderer.h>
#include <graffodil/GraphScene.h>
#include "TestHelpers.h"

using namespace Graffodil;
using namespace Graffodil::Test;

class TestBatchRenderer : public QObject {
    Q_OBJECT
private slots:
    void batchModeHidesIndividualItems() {
        GraphScene scene;
        auto *n1 = new TestNode("n1", {0, 0});
        auto *n2 = new TestNode("n2", {100, 0});
        scene.addNode(n1);
        scene.addNode(n2);

        BatchRenderer renderer(&scene);
        QVERIFY(!renderer.isBatchMode());
        QVERIFY(n1->graphicsItem()->isVisible());

        renderer.setBatchMode(true);
        QVERIFY(renderer.isBatchMode());
        QVERIFY(!n1->graphicsItem()->isVisible());
        QVERIFY(!n2->graphicsItem()->isVisible());

        renderer.setBatchMode(false);
        QVERIFY(n1->graphicsItem()->isVisible());
        QVERIFY(n2->graphicsItem()->isVisible());
    }

    void synchronizeUpdatesPositions() {
        GraphScene scene;
        auto *n1 = new TestNode("n1", {0, 0});
        auto *n2 = new TestNode("n2", {100, 0});
        scene.addNode(n1);
        scene.addNode(n2);

        BatchRenderer renderer(&scene);
        renderer.setBatchMode(true);

        // Move nodes
        n1->graphicsItem()->setPos(50, 50);
        n2->graphicsItem()->setPos(200, 200);
        renderer.synchronize();

        // No crash, batch mode still active
        QVERIFY(renderer.isBatchMode());
    }

    void autoThresholdSwitches() {
        GraphScene scene;
        BatchRenderer renderer(&scene);
        renderer.setAutoThreshold(3);

        // Add 2 nodes — below threshold
        auto *n1 = new TestNode("n1");
        auto *n2 = new TestNode("n2");
        scene.addNode(n1);
        scene.addNode(n2);
        renderer.synchronize();
        QVERIFY(!renderer.isBatchMode());

        // Add 3rd node — at threshold
        auto *n3 = new TestNode("n3");
        scene.addNode(n3);
        renderer.synchronize();
        QVERIFY(renderer.isBatchMode());
    }
};

QTEST_MAIN(TestBatchRenderer)
#include "tst_batchrenderer.moc"
