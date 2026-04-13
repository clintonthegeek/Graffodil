#include <QTest>
#include <graffodil/EdgePathStrategy.h>

using namespace Graffodil;

class TestEdgePathStrategies : public QObject {
    Q_OBJECT
private slots:
    void directPath() {
        DirectPathStrategy strategy;
        QPointF src(0, 0), tgt(100, 0);
        auto path = strategy.computePath(src, Side::Right, tgt, Side::Left);
        QVERIFY(!path.isEmpty());
        QCOMPARE(path.elementAt(0).x, 0.0);
        QCOMPARE(path.elementAt(0).y, 0.0);
        // Direct path ends at target
        auto last = path.elementAt(path.elementCount() - 1);
        QCOMPARE(last.x, 100.0);
        QCOMPARE(last.y, 0.0);
    }

    void bezierPath() {
        BezierPathStrategy strategy;
        QPointF src(0, 0), tgt(200, 0);
        auto path = strategy.computePath(src, Side::Right, tgt, Side::Left);
        QVERIFY(!path.isEmpty());
        // Bezier path has cubic elements (not just lineTo)
        bool hasCubic = false;
        for (int i = 0; i < path.elementCount(); ++i) {
            if (path.elementAt(i).type == QPainterPath::CurveToElement)
                hasCubic = true;
        }
        QVERIFY(hasCubic);
    }

    void bezierCurvatureCapped() {
        BezierPathStrategy strategy;
        // Very far apart — curvature should cap at 80.0
        QPointF src(0, 0), tgt(1000, 0);
        auto path = strategy.computePath(src, Side::Right, tgt, Side::Left);
        // Control point should be at most 80px from source
        // Element 1 is the first control point of cubicTo
        auto ctrl1 = path.elementAt(1);
        QVERIFY(ctrl1.x <= 80.0 + 0.1);
    }

    void orthogonalPath() {
        OrthogonalPathStrategy strategy;
        QPointF src(0, 50), tgt(200, 150);
        auto path = strategy.computePath(src, Side::Right, tgt, Side::Left);
        QVERIFY(!path.isEmpty());
        // Orthogonal: all segments are axis-aligned
        for (int i = 1; i < path.elementCount(); ++i) {
            auto prev = path.elementAt(i - 1);
            auto cur = path.elementAt(i);
            if (cur.type == QPainterPath::LineToElement) {
                bool horizontal = qFuzzyCompare(prev.y, cur.y);
                bool vertical = qFuzzyCompare(prev.x, cur.x);
                QVERIFY2(horizontal || vertical,
                    qPrintable(QString("Segment %1 is diagonal: (%2,%3)->(%4,%5)")
                        .arg(i).arg(prev.x).arg(prev.y).arg(cur.x).arg(cur.y)));
            }
        }
    }

    void polylineThroughWaypoints() {
        PolylinePathStrategy strategy;
        QPointF src(0, 0), tgt(300, 0);
        QList<QPointF> waypoints = {{100, 50}, {200, 50}};
        auto path = strategy.computePath(src, Side::Right, tgt, Side::Left, waypoints);
        QVERIFY(!path.isEmpty());
        // Should pass through waypoints
        bool found100 = false, found200 = false;
        for (int i = 0; i < path.elementCount(); ++i) {
            auto e = path.elementAt(i);
            if (qFuzzyCompare(e.x, 100.0) && qFuzzyCompare(e.y, 50.0))
                found100 = true;
            if (qFuzzyCompare(e.x, 200.0) && qFuzzyCompare(e.y, 50.0))
                found200 = true;
        }
        QVERIFY(found100);
        QVERIFY(found200);
    }
};

QTEST_MAIN(TestEdgePathStrategies)
#include "tst_edgepathstrategies.moc"
