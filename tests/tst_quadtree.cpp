#include <QTest>
#include <graffodil/QuadTree.h>
#include <cmath>

using namespace Graffodil;

class TestQuadTree : public QObject {
    Q_OBJECT
private slots:
    void repulsionPushesApart() {
        QuadTree tree;
        QVector<QPointF> positions = {{0, 0}, {10, 0}};
        QVector<qreal> masses = {1.0, 1.0};
        tree.build(positions, masses);

        // Node at (0,0) should be pushed left (away from node at (10,0))
        QPointF force = tree.computeRepulsion({0, 0}, 1000.0, 1.0, 0.8);
        QVERIFY(force.x() < 0);  // pushed left
    }

    void farNodesApproximated() {
        // With theta > 0, distant groups are treated as single bodies
        QuadTree tree;
        QVector<QPointF> positions;
        QVector<qreal> masses;
        // Cluster at (0,0)
        for (int i = 0; i < 10; ++i) {
            positions.append({double(i), 0});
            masses.append(1.0);
        }
        // Cluster far away
        for (int i = 0; i < 10; ++i) {
            positions.append({1000.0 + double(i), 0});
            masses.append(1.0);
        }
        tree.build(positions, masses);

        // Should still produce a valid force (not crash, not NaN)
        QPointF force = tree.computeRepulsion({5, 0}, 1000.0, 1.0, 0.8);
        QVERIFY(!std::isnan(force.x()));
        QVERIFY(!std::isnan(force.y()));
    }

    void singleNode() {
        QuadTree tree;
        QVector<QPointF> positions = {{50, 50}};
        QVector<qreal> masses = {1.0};
        tree.build(positions, masses);

        // Single node repulsion from itself — should be zero or near-zero
        QPointF force = tree.computeRepulsion({50, 50}, 1000.0, 1.0, 0.8);
        QVERIFY(std::abs(force.x()) < 1.0);
        QVERIFY(std::abs(force.y()) < 1.0);
    }
};

QTEST_MAIN(TestQuadTree)
#include "tst_quadtree.moc"
