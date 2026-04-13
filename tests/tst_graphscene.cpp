#include <QTest>
#include "TestHelpers.h"

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
};

QTEST_MAIN(TestInterfaces)
#include "tst_graphscene.moc"
