#include <QTest>
#include <QSignalSpy>
#include "TestHelpers.h"
#include <graffodil/GraphScene.h>

using namespace Graffodil;
using namespace Graffodil::Test;

class TestInterfaces : public QObject {
    Q_OBJECT
private slots:
    void testNodeInterface() {
        TestNode node("n1", {10, 20});
        QCOMPARE(node.nodeId(), "n1");
        QCOMPARE(node.anchorPoint(Side::Top).x(), 60.0);  // 10 + 100/2
        QCOMPARE(node.anchorPoint(Side::Top).y(), 20.0);
        QCOMPARE(node.anchorPoint(Side::Right).x(), 110.0); // 10 + 100
        QVERIFY(node.graphicsItem() != nullptr);
    }

    void testEdgeInterface() {
        TestNode src("src"), tgt("tgt");
        TestEdge edge("e1", &src, &tgt);
        QCOMPARE(edge.edgeId(), "e1");
        QCOMPARE(edge.sourceNode()->nodeId(), "src");
        QCOMPARE(edge.targetNode()->nodeId(), "tgt");
        QVERIFY(edge.graphicsItem() != nullptr);
    }

    void testLayoutResult() {
        LayoutResult result;
        result.nodePositions["n1"] = {100, 200};
        QCOMPARE(result.nodePositions.size(), 1);
        QCOMPARE(result.nodePositions["n1"], QPointF(100, 200));
    }

    void testAddRemoveNodes() {
        GraphScene scene;
        auto *n1 = new TestNode("n1");
        auto *n2 = new TestNode("n2");

        scene.addNode(n1);
        scene.addNode(n2);
        QCOMPARE(scene.nodes().size(), 2);
        QCOMPARE(scene.nodeForId("n1"), n1);
        QCOMPARE(scene.nodeForId("n2"), n2);
        QVERIFY(scene.nodeForId("n3") == nullptr);

        scene.removeNode("n1");
        QCOMPARE(scene.nodes().size(), 1);
        QVERIFY(scene.nodeForId("n1") == nullptr);
    }

    void testAddRemoveEdges() {
        GraphScene scene;
        auto *n1 = new TestNode("n1");
        auto *n2 = new TestNode("n2");
        scene.addNode(n1);
        scene.addNode(n2);

        auto *e1 = new TestEdge("e1", n1, n2);
        scene.addEdge(e1);
        QCOMPARE(scene.edges().size(), 1);
        QCOMPARE(scene.edgeForId("e1"), e1);
        QCOMPARE(scene.edgeBetween("n1", "n2"), e1);
        QVERIFY(scene.edgeBetween("n2", "n1") == nullptr);

        scene.removeEdge("e1");
        QCOMPARE(scene.edges().size(), 0);
    }

    void testNodeAddedSignal() {
        GraphScene scene;
        QSignalSpy spy(&scene, &GraphScene::nodeAdded);
        auto *n1 = new TestNode("n1");
        scene.addNode(n1);
        QCOMPARE(spy.count(), 1);
    }

    void testApplyLayout() {
        GraphScene scene;
        auto *n1 = new TestNode("n1", {0, 0});
        auto *n2 = new TestNode("n2", {0, 0});
        scene.addNode(n1);
        scene.addNode(n2);

        LayoutResult result;
        result.nodePositions["n1"] = {100, 200};
        result.nodePositions["n2"] = {300, 400};
        scene.applyLayout(result);

        QCOMPARE(n1->graphicsItem()->pos(), QPointF(100, 200));
        QCOMPARE(n2->graphicsItem()->pos(), QPointF(300, 400));
    }

    void testSelectedNodes() {
        GraphScene scene;
        auto *n1 = new TestNode("n1");
        auto *n2 = new TestNode("n2");
        n1->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
        n2->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
        scene.addNode(n1);
        scene.addNode(n2);

        n1->graphicsItem()->setSelected(true);
        QCOMPARE(scene.selectedNodes().size(), 1);
        QCOMPARE(scene.selectedNodes().first(), n1);
    }
};

QTEST_MAIN(TestInterfaces)
#include "tst_graphscene.moc"
