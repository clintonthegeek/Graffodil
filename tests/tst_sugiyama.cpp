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

    // Punch list (ASK-M19): topologicalSort() returns a partial order (or the
    // all-nodes fallback) when a component has a cycle, so the single
    // longest-path pass skipped a predecessor that had not been visited yet and
    // left both endpoints of an edge in the same layer — span 0, no waypoints.
    // Here Y has a self-loop and an edge to X: Y must sit one layer left of X.
    void selfLoopDoesNotCollapseItsEdge()
    {
        TestNode x("X"), y("Y");
        TestEdge selfLoop("loop", &y, &y);
        TestEdge yToX("yx", &y, &x);

        SugiyamaLayout layout;
        layout.setNodeSize({100, 60});
        layout.setSpacing(60, 40);
        layout.setNodes({&x, &y});
        layout.setEdges({&selfLoop, &yToX});

        const auto result = layout.compute();
        QCOMPARE(result.nodePositions.size(), 2);
        QVERIFY2(result.nodePositions["X"].x() > result.nodePositions["Y"].x(),
                 qPrintable(QStringLiteral("X at %1, Y at %2")
                                .arg(result.nodePositions["X"].x())
                                .arg(result.nodePositions["Y"].x())));
    }

    // A cycle elsewhere must not disturb a plain chain's layering.
    void cycleDoesNotDisturbTheChain()
    {
        TestNode a("A"), b("B"), c("C"), loop("S");
        TestEdge e1("e1", &a, &b), e2("e2", &b, &c);
        TestEdge selfLoop("loop", &loop, &loop), loopOut("out", &loop, &c);

        SugiyamaLayout layout;
        layout.setNodeSize({100, 60});
        layout.setSpacing(60, 40);
        layout.setNodes({&a, &b, &c, &loop});
        layout.setEdges({&e1, &e2, &selfLoop, &loopOut});

        const auto result = layout.compute();
        QCOMPARE(result.nodePositions.size(), 4);
        QVERIFY(result.nodePositions["A"].x() < result.nodePositions["B"].x());
        QVERIFY(result.nodePositions["B"].x() < result.nodePositions["C"].x());
        QVERIFY(std::isfinite(result.nodePositions["S"].x()));
    }

    // A 2-cycle plus a chain: the chain's layering is still one layer per edge
    // and every node gets a finite layer.
    void twoCycleKeepsTheChainLayered()
    {
        TestNode a("A"), b("B"), c("C");
        TestNode x("X"), y("Y");
        TestEdge e1("e1", &a, &b), e2("e2", &b, &c);
        TestEdge xy("xy", &x, &y), yx("yx", &y, &x);

        SugiyamaLayout layout;
        layout.setNodeSize({100, 60});
        layout.setSpacing(60, 40);
        layout.setNodes({&a, &b, &c, &x, &y});
        layout.setEdges({&e1, &e2, &xy, &yx});

        const auto result = layout.compute();
        QCOMPARE(result.nodePositions.size(), 5);
        QVERIFY(result.nodePositions["A"].x() < result.nodePositions["B"].x());
        QVERIFY(result.nodePositions["B"].x() < result.nodePositions["C"].x());
        for (const QPointF &p : result.nodePositions) {
            QVERIFY(std::isfinite(p.x()));
            QVERIFY(std::isfinite(p.y()));
        }
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

    void defaultDirectionIsLeftToRight() {
        SugiyamaLayout layout;
        QCOMPARE(layout.direction(), SugiyamaLayout::Direction::LeftToRight);
    }

    void rightToLeftMirrorsXAxis() {
        // A -> B -> C: in LR, A has smallest X; in RL, A has largest X.
        TestNode a("A"), b("B"), c("C");
        TestEdge e1("e1", &a, &b), e2("e2", &b, &c);

        SugiyamaLayout lr;
        lr.setNodeSize({100, 60});
        lr.setSpacing(60, 40);
        lr.setNodes({&a, &b, &c});
        lr.setEdges({&e1, &e2});
        auto rLR = lr.compute();

        SugiyamaLayout rl;
        rl.setNodeSize({100, 60});
        rl.setSpacing(60, 40);
        rl.setDirection(SugiyamaLayout::Direction::RightToLeft);
        rl.setNodes({&a, &b, &c});
        rl.setEdges({&e1, &e2});
        auto rRL = rl.compute();

        // Layer ordering is reversed along X
        QVERIFY(rRL.nodePositions["A"].x() > rRL.nodePositions["B"].x());
        QVERIFY(rRL.nodePositions["B"].x() > rRL.nodePositions["C"].x());

        // RL's positions are the LR positions mirrored about the layout's
        // X center: A's X in RL == C's X in LR (layer 0 <-> layer maxLayer).
        QCOMPARE(rRL.nodePositions["A"].x(), rLR.nodePositions["C"].x());
        QCOMPARE(rRL.nodePositions["C"].x(), rLR.nodePositions["A"].x());

        // Y axis is untouched.
        QCOMPARE(rRL.nodePositions["A"].y(), rLR.nodePositions["A"].y());
    }

    void topToBottomSwapsAxes() {
        // A -> B -> C: in TB, A should have smallest Y, C largest.
        TestNode a("A"), b("B"), c("C");
        TestEdge e1("e1", &a, &b), e2("e2", &b, &c);

        SugiyamaLayout layout;
        layout.setNodeSize({100, 60});
        layout.setSpacing(60, 40);
        layout.setDirection(SugiyamaLayout::Direction::TopToBottom);
        layout.setNodes({&a, &b, &c});
        layout.setEdges({&e1, &e2});

        auto result = layout.compute();
        QVERIFY(result.nodePositions["A"].y() < result.nodePositions["B"].y());
        QVERIFY(result.nodePositions["B"].y() < result.nodePositions["C"].y());
        // Chain of three should share the same X (all in one within-layer "column").
        QCOMPARE(result.nodePositions["A"].x(), result.nodePositions["B"].x());
        QCOMPARE(result.nodePositions["B"].x(), result.nodePositions["C"].x());
    }

    void bottomToTopMirrorsYAxis() {
        TestNode a("A"), b("B"), c("C");
        TestEdge e1("e1", &a, &b), e2("e2", &b, &c);

        SugiyamaLayout tb;
        tb.setNodeSize({100, 60});
        tb.setSpacing(60, 40);
        tb.setDirection(SugiyamaLayout::Direction::TopToBottom);
        tb.setNodes({&a, &b, &c});
        tb.setEdges({&e1, &e2});
        auto rTB = tb.compute();

        SugiyamaLayout bt;
        bt.setNodeSize({100, 60});
        bt.setSpacing(60, 40);
        bt.setDirection(SugiyamaLayout::Direction::BottomToTop);
        bt.setNodes({&a, &b, &c});
        bt.setEdges({&e1, &e2});
        auto rBT = bt.compute();

        // Chain ordering along Y is reversed
        QVERIFY(rBT.nodePositions["A"].y() > rBT.nodePositions["B"].y());
        QVERIFY(rBT.nodePositions["B"].y() > rBT.nodePositions["C"].y());

        // Mirror identity: A's Y in BT equals C's Y in TB.
        QCOMPARE(rBT.nodePositions["A"].y(), rTB.nodePositions["C"].y());
        QCOMPARE(rBT.nodePositions["C"].y(), rTB.nodePositions["A"].y());
    }

    void waypointsFollowDirection() {
        // A -> C with a 2-layer gap (via B) but only the A->C edge: the
        // long edge generates a dummy waypoint. For TB that waypoint
        // must lie between A and C on Y (not X).
        TestNode a("A"), b("B"), c("C");
        TestEdge ab("ab", &a, &b), bc("bc", &b, &c), ac("ac", &a, &c);

        SugiyamaLayout layout;
        layout.setNodeSize({100, 60});
        layout.setSpacing(60, 40);
        layout.setDirection(SugiyamaLayout::Direction::TopToBottom);
        layout.setNodes({&a, &b, &c});
        layout.setEdges({&ab, &bc, &ac});

        auto result = layout.compute();
        auto key = qMakePair(QString("A"), QString("C"));
        QVERIFY(result.edgeWaypoints.contains(key));
        const auto &wps = result.edgeWaypoints[key];
        QCOMPARE(wps.size(), 1);

        // Waypoint Y should sit between A and C (layer axis is Y in TB).
        const qreal wpY = wps.first().y();
        QVERIFY(wpY > result.nodePositions["A"].y());
        QVERIFY(wpY < result.nodePositions["C"].y());
    }

    void directionDoesNotBreakNonOverlap() {
        // The "no overlap" invariant must hold in every direction.
        TestNode a("A"), b("B"), c("C"), d("D");
        TestEdge e1("e1", &a, &b), e2("e2", &a, &c),
                 e3("e3", &b, &d), e4("e4", &c, &d);

        for (auto dir : {SugiyamaLayout::Direction::LeftToRight,
                         SugiyamaLayout::Direction::RightToLeft,
                         SugiyamaLayout::Direction::TopToBottom,
                         SugiyamaLayout::Direction::BottomToTop}) {
            SugiyamaLayout layout;
            layout.setNodeSize({100, 60});
            layout.setSpacing(60, 40);
            layout.setDirection(dir);
            layout.setNodes({&a, &b, &c, &d});
            layout.setEdges({&e1, &e2, &e3, &e4});
            auto r = layout.compute();
            QCOMPARE(r.nodePositions.size(), 4);

            // Rects must not overlap.
            const qreal w = 100.0, h = 60.0;
            auto ps = r.nodePositions.values();
            for (int i = 0; i < ps.size(); ++i) {
                QRectF ri(ps[i], QSizeF(w, h));
                for (int j = i + 1; j < ps.size(); ++j) {
                    QRectF rj(ps[j], QSizeF(w, h));
                    QVERIFY2(!ri.intersects(rj),
                             "Overlapping node rects");
                }
            }
        }
    }
};

QTEST_MAIN(TestSugiyamaLayout)
#include "tst_sugiyama.moc"
