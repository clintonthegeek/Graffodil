#include <QTest>
#include <QPainter>
#include <QImage>
#include <graffodil/TerminusStyle.h>

using namespace Graffodil;

class TestTerminus : public QObject {
    Q_OBJECT
private slots:
    void triangleInset() {
        TriangleTerminus term;
        QImage img(100, 100, QImage::Format_ARGB32);
        QPainter painter(&img);
        QPen pen(Qt::black, 2);

        QPointF inset = term.draw(&painter, {80, 50}, {20, 50}, pen);
        QVERIFY(inset.x() < 80.0);
        QCOMPARE(inset.y(), 50.0);
    }

    void noTerminusPassthrough() {
        NoTerminus term;
        QImage img(100, 100, QImage::Format_ARGB32);
        QPainter painter(&img);
        QPen pen(Qt::black, 2);

        QPointF tip(80, 50);
        QPointF inset = term.draw(&painter, tip, {20, 50}, pen);
        QCOMPARE(inset, tip);
    }

    void terminusSizePositive() {
        TriangleTerminus triangle;
        OpenArrowTerminus openArrow;
        DiamondTerminus diamond;
        CircleTerminus circle;
        BarTerminus bar;
        NoTerminus none;

        QVERIFY(triangle.size() > 0);
        QVERIFY(openArrow.size() > 0);
        QVERIFY(diamond.size() > 0);
        QVERIFY(circle.size() > 0);
        QVERIFY(bar.size() > 0);
        QCOMPARE(none.size(), 0.0);
    }

    void diamondFillToggle() {
        DiamondTerminus filled(true);
        DiamondTerminus unfilled(false);
        QImage img(100, 100, QImage::Format_ARGB32);
        QPainter painter(&img);
        QPen pen(Qt::black, 2);

        QPointF inset1 = filled.draw(&painter, {80, 50}, {20, 50}, pen);
        QPointF inset2 = unfilled.draw(&painter, {80, 50}, {20, 50}, pen);
        QVERIFY(inset1.x() < 80.0);
        QVERIFY(inset2.x() < 80.0);
    }
};

QTEST_MAIN(TestTerminus)
#include "tst_terminus.moc"
