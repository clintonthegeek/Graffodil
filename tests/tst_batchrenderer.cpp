#include <QTest>
#include <graffodil/BatchRenderer.h>
#include <graffodil/GraphScene.h>
#include <graffodil/IRenderableCache.h>
#include <QImage>
#include <QPainter>
#include "TestHelpers.h"
#include "BatchNodeItem.h"
#include "BatchEdgeItem.h"

using namespace Graffodil;
using namespace Graffodil::Test;

class CachedNode : public Graffodil::Test::TestNode, public Graffodil::IRenderableCache {
public:
    CachedNode(const QString &id, const QPointF &pos)
        : TestNode(id, pos)
    {
        m_pix = QPixmap(50, 30);
        m_pix.fill(Qt::red);
    }
    QPixmap cachedPixmap(const QSize &) override { return m_pix; }
    QSize contentSize() const override { return QSize(50, 30); }
    void invalidateCache() override { m_pix = {}; }
private:
    QPixmap m_pix;
};

class TestBatchRenderer : public QObject {
    Q_OBJECT
private slots:
    void batchModeHidesIndividualItems() {
        GraphScene scene;
        auto *n1 = new TestNode("n1", {0, 0});
        auto *n2 = new TestNode("n2", {100, 0});
        scene.addNode(n1);
        scene.addNode(n2);

        BatchRenderer renderer(&scene);
        QVERIFY(!renderer.isBatchMode());
        QVERIFY(n1->graphicsItem()->isVisible());

        renderer.setBatchMode(true);
        QVERIFY(renderer.isBatchMode());
        QVERIFY(!n1->graphicsItem()->isVisible());
        QVERIFY(!n2->graphicsItem()->isVisible());

        renderer.setBatchMode(false);
        QVERIFY(n1->graphicsItem()->isVisible());
        QVERIFY(n2->graphicsItem()->isVisible());
    }

    void synchronizeUpdatesPositions() {
        GraphScene scene;
        auto *n1 = new TestNode("n1", {0, 0});
        auto *n2 = new TestNode("n2", {100, 0});
        scene.addNode(n1);
        scene.addNode(n2);

        BatchRenderer renderer(&scene);
        renderer.setBatchMode(true);

        // Move nodes
        n1->graphicsItem()->setPos(50, 50);
        n2->graphicsItem()->setPos(200, 200);
        renderer.synchronize();

        // No crash, batch mode still active
        QVERIFY(renderer.isBatchMode());
    }

    void autoThresholdSwitches() {
        GraphScene scene;
        BatchRenderer renderer(&scene);
        renderer.setAutoThreshold(3);

        // Add 2 nodes — below threshold
        auto *n1 = new TestNode("n1");
        auto *n2 = new TestNode("n2");
        scene.addNode(n1);
        scene.addNode(n2);
        renderer.synchronize();
        QVERIFY(!renderer.isBatchMode());

        // Add 3rd node — at threshold
        auto *n3 = new TestNode("n3");
        scene.addNode(n3);
        renderer.synchronize();
        QVERIFY(renderer.isBatchMode());
    }

    void tightNodeBounds()
    {
        Graffodil::BatchNodeItem item;
        QVector<Graffodil::BatchNodeItem::NodeData> nodes(2);
        nodes[0].position = QPointF(0, 0);
        nodes[0].radius = 10;
        nodes[1].position = QPointF(15000, 300);   // beyond the old ±10000 rect
        nodes[1].radius = 10;
        item.setNodes(nodes);

        const QRectF b = item.boundingRect();
        QVERIFY(b.contains(QPointF(0, 0)));
        QVERIFY(b.contains(QPointF(15000, 300)));   // old code clipped this node
        QVERIFY(b.width() < 16000.0);               // and was 20000 wide regardless
    }

    void tightEdgeBounds()
    {
        Graffodil::BatchEdgeItem item;
        QVector<Graffodil::BatchEdgeItem::EdgeData> edges(1);
        edges[0].line = QLineF(QPointF(0, 0), QPointF(12000, 50));
        item.setEdges(edges);

        const QRectF b = item.boundingRect();
        QVERIFY(b.contains(QPointF(12000, 50)));
        QVERIFY(b.height() < 200.0);   // a near-horizontal line, not a 20000px band
    }

    void batchCompositesCachedPixmap() {
        GraphScene scene;
        auto *n1 = new CachedNode("n1", {100, 100});
        scene.addNode(n1);

        BatchRenderer renderer(&scene);
        renderer.setBatchMode(true);
        renderer.synchronize();

        // Paint the scene into an image and sample the pixel at the node centre.
        QImage img(300, 300, QImage::Format_ARGB32);
        img.fill(Qt::white);
        QPainter p(&img);
        scene.render(&p, QRectF(), QRectF(0, 0, 300, 300));
        p.end();

        const QRgb centre = img.pixel(100, 100);
        QCOMPARE(qRed(centre), 255);
        QVERIFY(qGreen(centre) < 50);
        QVERIFY(qBlue(centre) < 50);
    }
};

QTEST_MAIN(TestBatchRenderer)
#include "tst_batchrenderer.moc"
