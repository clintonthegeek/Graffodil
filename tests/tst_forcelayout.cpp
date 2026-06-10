#include <QTest>
#include <graffodil/ForceLayout.h>
#include "TestHelpers.h"
#include <cmath>

using namespace Graffodil;
using namespace Graffodil::Test;

class TestForceLayout : public QObject {
    Q_OBJECT
private slots:
    void convergesOnSmallGraph() {
        QList<TestNode *> nodes;
        for (int i = 0; i < 10; ++i)
            nodes.append(new TestNode(QString("n%1").arg(i)));

        // Chain: n0->n1->n2->...->n9
        QList<TestEdge *> edges;
        for (int i = 0; i < 9; ++i)
            edges.append(new TestEdge(QString("e%1").arg(i), nodes[i], nodes[i+1]));

        ForceLayout layout;
        QList<IGraphNode *> nodeList;
        for (auto *n : nodes) nodeList.append(n);
        layout.setNodes(nodeList);
        QList<IGraphEdge *> edgeList;
        for (auto *e : edges) edgeList.append(e);
        layout.setEdges(edgeList);

        auto result = layout.compute();
        QCOMPARE(result.nodePositions.size(), 10);

        qDeleteAll(edges);
        qDeleteAll(nodes);
    }

    void connectedNodesCloserThanDisconnected() {
        // Triangle A-B-C, plus isolated D
        TestNode a("A"), b("B"), c("C"), d("D");
        TestEdge e1("e1", &a, &b), e2("e2", &b, &c), e3("e3", &c, &a);

        ForceLayout layout;
        layout.setNodes({&a, &b, &c, &d});
        layout.setEdges({&e1, &e2, &e3});

        auto result = layout.compute();

        // Average distance between connected nodes should be less than
        // distance from connected nodes to isolated D
        auto dist = [&](const QString &x, const QString &y) {
            QPointF p1 = result.nodePositions[x];
            QPointF p2 = result.nodePositions[y];
            return std::sqrt(std::pow(p1.x()-p2.x(), 2) + std::pow(p1.y()-p2.y(), 2));
        };

        double connectedAvg = (dist("A","B") + dist("B","C") + dist("A","C")) / 3.0;
        double isolatedAvg = (dist("A","D") + dist("B","D") + dist("C","D")) / 3.0;
        QVERIFY(connectedAvg < isolatedAvg);
    }

    void pinnedNodeDoesNotMove() {
        TestNode a("A"), b("B"), c("C");
        TestEdge e1("e1", &a, &b), e2("e2", &b, &c);

        ForceLayout layout;
        layout.setNodes({&a, &b, &c});
        layout.setEdges({&e1, &e2});

        QPointF pinnedPos(500, 500);
        layout.pinNode("B", pinnedPos);

        auto result = layout.compute();
        QCOMPARE(result.nodePositions["B"], pinnedPos);
    }

    void tickReturnsFalseWhenConverged() {
        TestNode a("A"), b("B");
        TestEdge e1("e1", &a, &b);

        ForceLayout layout;
        layout.setNodes({&a, &b});
        layout.setEdges({&e1});

        // Run ticks until convergence
        int maxTicks = 5000;
        bool converged = false;
        for (int i = 0; i < maxTicks; ++i) {
            if (!layout.tick()) {
                converged = true;
                break;
            }
        }
        QVERIFY(converged);
    }

    void isIterativeTrue() {
        ForceLayout layout;
        QVERIFY(layout.isIterative());
    }

    void positionsStayFiniteAndBounded()
    {
        using namespace Graffodil;
        // Star graph — high-swinging scenario that exercises the speed cap.
        QList<Test::TestNode *> nodeStore;
        QList<IGraphNode *> nodes;
        QList<IGraphEdge *> edges;
        QList<Test::TestEdge *> edgeStore;
        auto *hub = new Test::TestNode("hub");
        nodeStore.append(hub);
        nodes.append(hub);
        for (int i = 0; i < 30; ++i) {
            auto *n = new Test::TestNode(QString("leaf%1").arg(i));
            nodeStore.append(n);
            nodes.append(n);
            auto *e = new Test::TestEdge(QString("e%1").arg(i), hub, n);
            edgeStore.append(e);
            edges.append(e);
        }

        ForceLayout layout;
        layout.setNodes(nodes);
        layout.setEdges(edges);
        for (int i = 0; i < 300 && layout.tick(); ++i) {}

        const LayoutResult r = layout.currentPositions();
        for (auto it = r.nodePositions.constBegin();
             it != r.nodePositions.constEnd(); ++it) {
            QVERIFY(std::isfinite(it.value().x()));
            QVERIFY(std::isfinite(it.value().y()));
            QVERIFY(qAbs(it.value().x()) < 1.0e6);
            QVERIFY(qAbs(it.value().y()) < 1.0e6);
        }

        qDeleteAll(edgeStore);
        qDeleteAll(nodeStore);
    }

    void tickIsDeterministicAcrossInstances()
    {
        using namespace Graffodil;
        auto build = [](QList<Test::TestNode *> &nodeStore,
                        QList<Test::TestEdge *> &edgeStore,
                        QList<IGraphNode *> &nodes, QList<IGraphEdge *> &edges) {
            for (int i = 0; i < 12; ++i) {
                auto *n = new Test::TestNode(QString("n%1").arg(i));
                nodeStore.append(n);
                nodes.append(n);
            }
            for (int i = 0; i < 12; ++i) {
                auto *e = new Test::TestEdge(QString("e%1").arg(i),
                                             nodes[i], nodes[(i + 1) % 12]);
                edgeStore.append(e);
                edges.append(e);
            }
            auto *chord = new Test::TestEdge("chord", nodes[0], nodes[6]);
            edgeStore.append(chord);
            edges.append(chord);
        };

        QList<Test::TestNode *> ns1, ns2;
        QList<Test::TestEdge *> es1, es2;
        QList<IGraphNode *> n1, n2;
        QList<IGraphEdge *> e1, e2;
        build(ns1, es1, n1, e1);
        build(ns2, es2, n2, e2);

        ForceLayout l1, l2;
        l1.setNodes(n1); l1.setEdges(e1);
        l2.setNodes(n2); l2.setEdges(e2);
        for (int i = 0; i < 50; ++i) { l1.tick(); l2.tick(); }

        const auto p1 = l1.currentPositions().nodePositions;
        const auto p2 = l2.currentPositions().nodePositions;
        QCOMPARE(p1.size(), p2.size());
        for (auto it = p1.constBegin(); it != p1.constEnd(); ++it)
            QCOMPARE(it.value(), p2.value(it.key()));

        qDeleteAll(es1); qDeleteAll(es2); qDeleteAll(ns1); qDeleteAll(ns2);
    }

    void energyConvergenceIsPlateauNotDecrease()
    {
        using namespace Graffodil;
        // A 20-node chain takes well over 15 ticks to physically settle from
        // BFS initial placement. The pre-fix heuristic stops after ~10 ticks of
        // monotonically decreasing energy — long before the layout has settled.
        QList<Test::TestNode *> nodeStore;
        QList<IGraphNode *> nodes;
        QList<IGraphEdge *> edges;
        QList<Test::TestEdge *> edgeStore;
        for (int i = 0; i < 20; ++i) {
            auto *n = new Test::TestNode(QString("n%1").arg(i));
            nodeStore.append(n);
            nodes.append(n);
        }
        for (int i = 0; i + 1 < 20; ++i) {
            auto *e = new Test::TestEdge(QString("e%1").arg(i),
                                         nodes[i], nodes[i + 1]);
            edgeStore.append(e);
            edges.append(e);
        }

        ForceLayout layout;
        layout.setNodes(nodes);
        layout.setEdges(edges);

        int ticks = 0;
        while (layout.tick() && ticks < 5000)
            ++ticks;

        QVERIFY(layout.isConverged());
        QVERIFY2(ticks > 15,
                 "simulation stopped while energy was still actively decreasing");

        qDeleteAll(edgeStore);
        qDeleteAll(nodeStore);
    }
};

QTEST_MAIN(TestForceLayout)
#include "tst_forcelayout.moc"
