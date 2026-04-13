#include <QTest>
#include <graffodil/CircularLayout.h>
#include <graffodil/GraphScene.h>
#include "TestHelpers.h"
#include <cmath>

using namespace Graffodil;
using namespace Graffodil::Test;

class TestCircularLayout : public QObject {
    Q_OBJECT
private slots:
    void threeNodesFormTriangle() {
        TestNode n1("n1"), n2("n2"), n3("n3");
        CircularLayout layout;
        layout.setRadius(100);
        layout.setNodes({&n1, &n2, &n3});
        layout.setEdges({});

        auto result = layout.compute();
        QCOMPARE(result.nodePositions.size(), 3);

        // All nodes should be ~100px from origin
        for (const auto &pos : result.nodePositions) {
            double dist = std::sqrt(pos.x() * pos.x() + pos.y() * pos.y());
            QVERIFY(std::abs(dist - 100.0) < 1.0);
        }
    }

    void centerNodeAtOrigin() {
        TestNode n1("center"), n2("a"), n3("b"), n4("c");
        CircularLayout layout;
        layout.setRadius(100);
        layout.setCenterNode("center");
        layout.setNodes({&n1, &n2, &n3, &n4});
        layout.setEdges({});

        auto result = layout.compute();
        QCOMPARE(result.nodePositions["center"], QPointF(0, 0));

        // Other 3 nodes on the circle
        for (const auto &id : {"a", "b", "c"}) {
            auto pos = result.nodePositions[id];
            double dist = std::sqrt(pos.x() * pos.x() + pos.y() * pos.y());
            QVERIFY(std::abs(dist - 100.0) < 1.0);
        }
    }

    void singleNodeAtOrigin() {
        TestNode n1("solo");
        CircularLayout layout;
        layout.setRadius(100);
        layout.setNodes({&n1});
        layout.setEdges({});

        auto result = layout.compute();
        QCOMPARE(result.nodePositions["solo"], QPointF(0, 0));
    }

    void isNotIterative() {
        CircularLayout layout;
        QVERIFY(!layout.isIterative());
    }
};

QTEST_MAIN(TestCircularLayout)
#include "tst_circular.moc"
