#include <algorithm>
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

    // A BatchRenderer is parented to the scene, so ~QGraphicsScene destroys
    // the items and only then deletes the renderer (a child QObject). Its
    // destructor must not walk items that are already gone.
    void sceneDeletedInBatchModeDoesNotTouchDestroyedItems() {
        auto *scene = new GraphScene;
        auto *n1 = new TestNode("n1", {0, 0});
        auto *n2 = new TestNode("n2", {100, 0});
        scene->addNode(n1);
        scene->addNode(n2);
        scene->addEdge(new TestEdge("e1", n1, n2));

        auto *renderer = new BatchRenderer(scene);
        renderer->setBatchMode(true);
        QVERIFY(renderer->isBatchMode());

        delete scene;   // destroys the items, then the renderer child
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

    // M14: the uncached-node disc colour comes from the scene palette and
    // can be overridden per node by a provider.
    void batchNodeColorFollowsPaletteAndProvider() {
        GraphScene scene;
        auto *n1 = new TestNode("n1", {100, 100});
        scene.addNode(n1);

        BatchRenderer renderer(&scene);
        renderer.setBatchMode(true);
        renderer.synchronize();

        QImage img(300, 300, QImage::Format_ARGB32);
        auto sampleCentre = [&]() {
            img.fill(Qt::white);
            QPainter p(&img);
            scene.render(&p, QRectF(), QRectF(0, 0, 300, 300));
            p.end();
            return img.pixel(100, 100);
        };

        // Default palette batch node disc.
        const QRgb def = sampleCentre();
        QCOMPARE(qRed(def), 100);
        QCOMPARE(qGreen(def), 150);
        QCOMPARE(qBlue(def), 200);

        // scene.setPalette() re-themes batch mode immediately.
        Graffodil::Palette pal;
        pal.batchNode = QColor(0, 128, 0);
        scene.setPalette(pal);
        const QRgb themed = sampleCentre();
        QCOMPARE(qRed(themed), 0);
        QCOMPARE(qGreen(themed), 128);
        QCOMPARE(qBlue(themed), 0);

        // The provider wins over the palette.
        renderer.setNodeColorProvider([](const IGraphNode *) {
            return QColor(0, 0, 255);
        });
        const QRgb provided = sampleCentre();
        QCOMPARE(qRed(provided), 0);
        QCOMPARE(qGreen(provided), 0);
        QCOMPARE(qBlue(provided), 255);
    }

    // M14: BatchEdgeItem honours setEdgeColors (BatchRenderer passes the
    // scene palette through on every synchronize()).
    void batchEdgeItemHonorsColors() {
        BatchEdgeItem item;
        QVector<BatchEdgeItem::EdgeData> edges(1);
        edges[0].line = QLineF(QPointF(0, 150), QPointF(300, 150));
        item.setEdges(edges);

        QGraphicsScene scene;
        scene.addItem(&item);
        QImage img(300, 300, QImage::Format_ARGB32);
        auto sampleMidLine = [&]() {
            img.fill(Qt::white);
            QPainter p(&img);
            scene.render(&p, QRectF(), QRectF(0, 0, 300, 300));
            p.end();
            return img.pixel(150, 150);
        };

        // Historical default: translucent grey (alpha 60) over white — a
        // 0.8px cosmetic pen leaves partial AA coverage, so the blend sits
        // well above white but below an unrelated colour.
        const auto greenAt = [&img](int y) {
            return qGreen(img.pixel(150, y));
        };
        sampleMidLine();
        const int defG = std::min({greenAt(150), greenAt(149), greenAt(151)});
        QVERIFY(defG > 200);           // translucent, definitely not white
        QVERIFY(defG < 250);           // and not untouched background

        item.setEdgeColors(QColor(255, 0, 0), QColor(200, 200, 200, 20));
        sampleMidLine();
        const int themedG = std::min({greenAt(150), greenAt(149), greenAt(151)});
        QVERIFY(themedG < 150);        // opaque red replaced the grey
        QCOMPARE(qRed(img.pixel(150, 150)), 255);

        // Stack-allocated item: the scene must not try to delete it later.
        scene.removeItem(&item);
    }
};

QTEST_MAIN(TestBatchRenderer)
#include "tst_batchrenderer.moc"
