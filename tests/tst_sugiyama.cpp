#include <QTest>
#include <graffodil/SugiyamaLayout.h>
#include <graffodil/GraphScene.h>
#include "TestHelpers.h"
#include <cmath>

using namespace Graffodil;
using namespace Graffodil::Test;

class TestSugiyamaLayout : public QObject {
    Q_OBJECT
private slots:
    void linearChainProducesLayers() {
        // A -> B -> C should produce 3 columns, left to right
        TestNode a("A"), b("B"), c("C");
        TestEdge e1("e1", &a, &b), e2("e2", &b, &c);

        SugiyamaLayout layout;
        layout.setNodeSize({100, 60});
        layout.setSpacing(60, 40);
        layout.setNodes({&a, &b, &c});
        layout.setEdges({&e1, &e2});

        auto result = layout.compute();
        QCOMPARE(result.nodePositions.size(), 3);

        // A should be leftmost, C rightmost
        QVERIFY(result.nodePositions["A"].x() < result.nodePositions["B"].x());
        QVERIFY(result.nodePositions["B"].x() < result.nodePositions["C"].x());
    }

    void nodesDoNotOverlap() {
        TestNode a("A"), b("B"), c("C"), d("D");
        TestEdge e1("e1", &a, &b), e2("e2", &a, &c), e3("e3", &b, &d), e4("e4", &c, &d);

        SugiyamaLayout layout;
        layout.setNodeSize({100, 60});
        layout.setSpacing(60, 40);
        layout.setNodes({&a, &b, &c, &d});
        layout.setEdges({&e1, &e2, &e3, &e4});

        auto result = layout.compute();

        // Check no overlaps: for all pairs with same X, Y differs by at least nodeHeight
        auto positions = result.nodePositions.values();
        for (int i = 0; i < positions.size(); ++i) {
            for (int j = i + 1; j < positions.size(); ++j) {
                if (qFuzzyCompare(positions[i].x(), positions[j].x())) {
                    QVERIFY(std::abs(positions[i].y() - positions[j].y()) >= 60.0);
                }
            }
        }
    }

    void spineAlignmentCenters() {
        // A -> B -> C with spine set — spine nodes should have similar Y
        TestNode a("A"), b("B"), c("C");
        TestEdge e1("e1", &a, &b), e2("e2", &b, &c);

        SugiyamaLayout layout;
        layout.setNodeSize({100, 60});
        layout.setSpacing(60, 40);
        layout.setNodes({&a, &b, &c});
        layout.setEdges({&e1, &e2});
        layout.setSpine({"A", "B", "C"});
        layout.setSpineAlignment(true);

        auto result = layout.compute();

        // All spine nodes should have similar Y center
        qreal yA = result.nodePositions["A"].y() + 30;
        qreal yB = result.nodePositions["B"].y() + 30;
        qreal yC = result.nodePositions["C"].y() + 30;
        QVERIFY(std::abs(yA - yB) < 50.0);
        QVERIFY(std::abs(yB - yC) < 50.0);
    }

    void disconnectedComponentsHandled() {
        // Two separate subgraphs: {A->B} and {C->D}
        TestNode a("A"), b("B"), c("C"), d("D");
        TestEdge e1("e1", &a, &b), e2("e2", &c, &d);

        SugiyamaLayout layout;
        layout.setNodeSize({100, 60});
        layout.setSpacing(60, 40);
        layout.setNodes({&a, &b, &c, &d});
        layout.setEdges({&e1, &e2});

        auto result = layout.compute();
        QCOMPARE(result.nodePositions.size(), 4);

        // Both subgraphs should be positioned (all 4 nodes have positions)
        QVERIFY(result.nodePositions.contains("A"));
        QVERIFY(result.nodePositions.contains("B"));
        QVERIFY(result.nodePositions.contains("C"));
        QVERIFY(result.nodePositions.contains("D"));
    }

    void singleNodeWorks() {
        TestNode a("A");
        SugiyamaLayout layout;
        layout.setNodeSize({100, 60});
        layout.setSpacing(60, 40);
        layout.setNodes({&a});
        layout.setEdges({});

        auto result = layout.compute();
        QCOMPARE(result.nodePositions.size(), 1);
        QVERIFY(result.nodePositions.contains("A"));
    }

    void isNotIterative() {
        SugiyamaLayout layout;
        QVERIFY(!layout.isIterative());
    }
};

QTEST_MAIN(TestSugiyamaLayout)
#include "tst_sugiyama.moc"
