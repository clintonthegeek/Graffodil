#include <QTest>
#include <graffodil/SpatialGrid.h>

using namespace Graffodil;

class TestSpatialGrid : public QObject {
    Q_OBJECT
private slots:
    void queryReturnsVisibleIndices() {
        SpatialGrid grid;
        QVector<QPointF> positions = {{10, 10}, {50, 50}, {90, 90}, {150, 150}};
        grid.rebuild(positions, QRectF(0, 0, 200, 200));

        auto result = grid.query(QRectF(0, 0, 100, 100));
        // Should contain indices 0, 1, 2 (within viewport)
        QVERIFY(result.contains(0));
        QVERIFY(result.contains(1));
        QVERIFY(result.contains(2));
    }

    void emptyGridReturnsEmpty() {
        SpatialGrid grid;
        auto result = grid.query(QRectF(0, 0, 100, 100));
        QVERIFY(result.isEmpty());
    }

    void rebuildWithNewPositions() {
        SpatialGrid grid;
        QVector<QPointF> pos1 = {{10, 10}};
        grid.rebuild(pos1, QRectF(0, 0, 100, 100));
        QCOMPARE(grid.query(QRectF(0, 0, 50, 50)).size(), 1);

        QVector<QPointF> pos2 = {{80, 80}};
        grid.rebuild(pos2, QRectF(0, 0, 100, 100));
        QVERIFY(grid.query(QRectF(0, 0, 50, 50)).isEmpty());
        QCOMPARE(grid.query(QRectF(50, 50, 50, 50)).size(), 1);
    }
};

QTEST_MAIN(TestSpatialGrid)
#include "tst_spatialgrid.moc"
