#include <QTest>
#include <graffodil/EdgePathStrategy.h>
#include <graffodil/Anchors.h>

using namespace Graffodil;

// Helper to build compass anchors at a point.
static Anchor anchorAt(const QPointF &p, const QPointF &outwardDir,
                       const QString &id = "test")
{
    Anchor a;
    a.id = id;
    a.scenePos = p;
    a.outwardDirection = outwardDir;
    return a;
}

class TestEdgePathStrategies : public QObject {
    Q_OBJECT
private slots:
    void directPath() {
        DirectPathStrategy strategy;
        Anchor src = anchorAt({0, 0}, {1, 0});   // facing right
        Anchor tgt = anchorAt({100, 0}, {-1, 0}); // facing left
        auto path = strategy.computePath(src, tgt);
        QVERIFY(!path.isEmpty());
        QCOMPARE(path.elementAt(0).x, 0.0);
        QCOMPARE(path.elementAt(0).y, 0.0);
        auto last = path.elementAt(path.elementCount() - 1);
        QCOMPARE(last.x, 100.0);
        QCOMPARE(last.y, 0.0);
    }

    void bezierPath() {
        BezierPathStrategy strategy;
        Anchor src = anchorAt({0, 0}, {1, 0});
        Anchor tgt = anchorAt({200, 0}, {-1, 0});
        auto path = strategy.computePath(src, tgt);
        QVERIFY(!path.isEmpty());
        bool hasCubic = false;
        for (int i = 0; i < path.elementCount(); ++i) {
            if (path.elementAt(i).type == QPainterPath::CurveToElement)
                hasCubic = true;
        }
        QVERIFY(hasCubic);
    }

    void bezierCurvatureCapped() {
        BezierPathStrategy strategy;
        Anchor src = anchorAt({0, 0}, {1, 0});
        Anchor tgt = anchorAt({1000, 0}, {-1, 0});
        auto path = strategy.computePath(src, tgt);
        auto ctrl1 = path.elementAt(1);
        QVERIFY(ctrl1.x <= 80.0 + 0.1);
    }

    void orthogonalPath() {
        OrthogonalPathStrategy strategy;
        Anchor src = anchorAt({0, 50}, {1, 0});
        Anchor tgt = anchorAt({200, 150}, {-1, 0});
        auto path = strategy.computePath(src, tgt);
        QVERIFY(!path.isEmpty());
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
        Anchor src = anchorAt({0, 0}, {1, 0});
        Anchor tgt = anchorAt({300, 0}, {-1, 0});
        QList<QPointF> waypoints = {{100, 50}, {200, 50}};
        auto path = strategy.computePath(src, tgt, waypoints);
        QVERIFY(!path.isEmpty());
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
