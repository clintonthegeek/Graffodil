#include <QTest>
#include <QImage>
#include <QPainter>

#include <graffodil/GraphScene.h>
#include <graffodil/GraphEdgeItem.h>
#include <graffodil/EdgePathStrategy.h>

#include "TestHelpers.h"

using namespace Graffodil;
using Graffodil::Test::TestNode;

class TstDashOffset : public QObject {
    Q_OBJECT
private slots:
    void roundTripAndRepaint();
};

void TstDashOffset::roundTripAndRepaint()
{
    GraphScene scene;
    auto *a = new TestNode(QStringLiteral("a"), QPointF(0, 0));
    auto *b = new TestNode(QStringLiteral("b"), QPointF(300, 0));
    scene.addNode(a);
    scene.addNode(b);
    auto *edge = new GraphEdgeItem(a, QString(), b, QString(),
                                   std::make_unique<DirectPathStrategy>());
    scene.addEdge(edge);
    edge->adjust();

    QPen pen(QColor(0, 0, 255), 3.0);
    pen.setDashPattern({4.0, 4.0});
    edge->setPen(pen);

    QCOMPARE(edge->dashOffset(), 0.0);
    edge->setDashOffset(6.5);
    QCOMPARE(edge->dashOffset(), 6.5);

    // Render twice with different offsets — images must differ (the dash
    // pattern visibly shifted), proving the offset reaches the painter.
    auto render = [&scene]() {
        QImage img(420, 80, QImage::Format_ARGB32);
        img.fill(Qt::white);
        QPainter p(&img);
        scene.render(&p, img.rect(), QRectF(-10, -40, 420, 80));
        return img;
    };
    edge->setDashOffset(0.0);
    const QImage at0 = render();
    edge->setDashOffset(4.0);
    const QImage at4 = render();
    QVERIFY(at0 != at4);
}

QTEST_MAIN(TstDashOffset)
#include "tst_dashoffset.moc"
