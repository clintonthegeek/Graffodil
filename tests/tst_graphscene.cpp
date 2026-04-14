#include <QTest>
#include <QSignalSpy>
#include "TestHelpers.h"
#include <graffodil/GraphScene.h>
#include <graffodil/Anchors.h>
#include <graffodil/GraphEdgeItem.h>
#include <graffodil/EdgePathStrategy.h>

// A node that publishes arbitrary named anchors (like DAW plugin ports),
// plus overrides anchorToward() for swivel behavior on a subset.
class PortNode : public QGraphicsRectItem, public Graffodil::IGraphNode {
public:
    PortNode() : QGraphicsRectItem(0, 0, 120, 80) {}
    QString nodeId() const override { return "plugin"; }
    QList<Graffodil::Anchor> anchors() const override {
        const QRectF r = mapToScene(rect()).boundingRect();
        QList<Graffodil::Anchor> list;
        // Two audio outputs on the right, one sidechain input on the left.
        Graffodil::Anchor outL;
        outL.id = "audio_out_l";
        outL.scenePos = {r.right(), r.top() + r.height() / 3};
        outL.outwardDirection = {1, 0};
        outL.metadata = QString("audio");
        list.append(outL);
        Graffodil::Anchor outR;
        outR.id = "audio_out_r";
        outR.scenePos = {r.right(), r.top() + 2 * r.height() / 3};
        outR.outwardDirection = {1, 0};
        outR.metadata = QString("audio");
        list.append(outR);
        Graffodil::Anchor sc;
        sc.id = "sidechain_in";
        sc.scenePos = {r.left(), r.center().y()};
        sc.outwardDirection = {-1, 0};
        sc.metadata = QString("audio");
        list.append(sc);
        return list;
    }
    QRectF nodeBoundingRect() const override { return mapToScene(rect()).boundingRect(); }
    QGraphicsItem *graphicsItem() override { return this; }
};

using namespace Graffodil;
using namespace Graffodil::Test;

class TestInterfaces : public QObject {
    Q_OBJECT
private slots:
    void testNodeInterface() {
        TestNode node("n1", {10, 20});
        QCOMPARE(node.nodeId(), "n1");
        const auto top = node.anchor(AnchorIds::Top);
        QCOMPARE(top.scenePos.x(), 60.0);  // 10 + 100/2
        QCOMPARE(top.scenePos.y(), 20.0);
        const auto right = node.anchor(AnchorIds::Right);
        QCOMPARE(right.scenePos.x(), 110.0); // 10 + 100
        QCOMPARE(node.anchors().size(), 4);
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

    void testPortAnchors() {
        // A node with named ports (instead of compass anchors) works
        // end-to-end: anchor(), anchors(), anchorToward() all behave.
        PortNode node;
        QCOMPARE(node.anchors().size(), 3);
        QCOMPARE(node.anchor("audio_out_l").metadata.toString(), QString("audio"));
        QCOMPARE(node.anchor("sidechain_in").outwardDirection, QPointF(-1, 0));

        // anchorToward default picks nearest — far-right point should pick
        // one of the right-side outputs, not the sidechain.
        auto nearest = node.anchorToward({500, 30});
        QVERIFY(nearest.id.startsWith("audio_out"));
    }

    void testEdgeUsesNamedAnchors() {
        // An edge built with arbitrary anchor ids resolves them correctly.
        PortNode n1;
        PortNode n2;
        n2.setPos(300, 0);
        auto *edge = new GraphEdgeItem(&n1, "audio_out_r",
                                        &n2, "sidechain_in",
                                        new DirectPathStrategy);
        edge->adjust();
        QVERIFY(!edge->path().isEmpty());
        const QPointF start = edge->path().elementAt(0);
        const QPointF end   = edge->path().elementAt(edge->path().elementCount() - 1);
        // Start should be n1's audio_out_r, end should be n2's sidechain_in.
        QCOMPARE(start, n1.anchor("audio_out_r").scenePos);
        QCOMPARE(end,   n2.anchor("sidechain_in").scenePos);
        delete edge;
    }

    void testEdgeBetweenUndirected() {
        GraphScene scene;
        auto *a = new TestNode("a");
        auto *b = new TestNode("b");
        auto *c = new TestNode("c");
        scene.addNode(a);
        scene.addNode(b);
        scene.addNode(c);

        auto *e1 = new TestEdge("e1", a, b);  // directed a -> b
        scene.addEdge(e1);

        // Directed lookup respects order.
        QCOMPARE(scene.edgeBetween("a", "b"), e1);
        QVERIFY(scene.edgeBetween("b", "a") == nullptr);

        // Undirected lookup matches either order.
        QCOMPARE(scene.edgeBetweenUndirected("a", "b"), e1);
        QCOMPARE(scene.edgeBetweenUndirected("b", "a"), e1);

        // Non-existent pair still returns null.
        QVERIFY(scene.edgeBetweenUndirected("a", "c") == nullptr);
    }

    void testSwivelAnchorViaEmptyId() {
        // An edge endpoint with empty anchor id invokes anchorToward()
        // dynamically each adjust. Default anchorToward picks nearest.
        TestNode a("a", {0, 0});
        TestNode b("b", {300, 200});
        auto *edge = new GraphEdgeItem(&a, QString(),   // swivel source
                                        &b, QString(),  // swivel target
                                        new DirectPathStrategy);
        edge->adjust();
        QVERIFY(!edge->path().isEmpty());
    }
};

QTEST_MAIN(TestInterfaces)
#include "tst_graphscene.moc"
