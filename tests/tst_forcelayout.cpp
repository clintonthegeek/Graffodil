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
};

QTEST_MAIN(TestForceLayout)
#include "tst_forcelayout.moc"
