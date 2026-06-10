#include <QTest>
#include <graffodil/GraphEdgeItem.h>
#include <graffodil/EdgePathStrategy.h>
#include <graffodil/TerminusStyle.h>
#include <graffodil/GraphScene.h>
#include <graffodil/Anchors.h>
#include "TestHelpers.h"

using namespace Graffodil;
using namespace Graffodil::Test;

class TestGraphEdgeItem : public QObject {
    Q_OBJECT
private slots:
    void edgeImplementsInterface() {
        TestNode src("src", {0, 0}), tgt("tgt", {200, 0});
        GraphEdgeItem edge(&src, AnchorIds::Right, &tgt, AnchorIds::Left,
                           std::make_unique<DirectPathStrategy>());

        QVERIFY(!edge.edgeId().isEmpty());
        QCOMPARE(edge.sourceNode(), static_cast<IGraphNode *>(&src));
        QCOMPARE(edge.targetNode(), static_cast<IGraphNode *>(&tgt));
        QVERIFY(edge.graphicsItem() != nullptr);
    }

    void adjustUpdatesPath() {
        TestNode src("src", {0, 0}), tgt("tgt", {200, 0});
        GraphEdgeItem edge(&src, AnchorIds::Right, &tgt, AnchorIds::Left,
                           std::make_unique<DirectPathStrategy>());

        edge.adjust();
        QVERIFY(!edge.path().isEmpty());

        // Move target, readjust
        tgt.setPos(400, 0);
        edge.adjust();
        auto path = edge.path();
        auto last = path.elementAt(path.elementCount() - 1);
        QVERIFY(last.x > 300.0);
    }

    void setPathStrategyChangesShape() {
        TestNode src("src", {0, 0}), tgt("tgt", {200, 100});
        GraphEdgeItem edge(&src, AnchorIds::Right, &tgt, AnchorIds::Left,
                           std::make_unique<DirectPathStrategy>());
        edge.adjust();
        int directElements = edge.path().elementCount();

        edge.setPathStrategy(std::make_unique<BezierPathStrategy>());
        edge.adjust();
        int bezierElements = edge.path().elementCount();

        QVERIFY(bezierElements > directElements);
    }

    void terminusOwnership() {
        TestNode src("src", {0, 0}), tgt("tgt", {200, 0});
        GraphEdgeItem edge(&src, AnchorIds::Right, &tgt, AnchorIds::Left,
                           std::make_unique<DirectPathStrategy>());

        edge.setTerminus(ArrowEnd::Target, std::make_unique<TriangleTerminus>());
        edge.setTerminus(ArrowEnd::Source, std::make_unique<NoTerminus>());
        edge.adjust();
        QVERIFY(!edge.path().isEmpty());
    }

    void edgeInScene() {
        GraphScene scene;
        auto *src = new TestNode("src", {0, 0});
        auto *tgt = new TestNode("tgt", {200, 0});
        scene.addNode(src);
        scene.addNode(tgt);

        auto *edge = new GraphEdgeItem(src, AnchorIds::Right, tgt, AnchorIds::Left,
                                       std::make_unique<DirectPathStrategy>());
        scene.addEdge(edge);

        QCOMPARE(scene.edges().size(), 1);
        QCOMPARE(scene.edgeBetween("src", "tgt"), static_cast<IGraphEdge *>(edge));
    }

    void shapeFollowsAdjust()
    {
        using namespace Graffodil;
        auto *src = new Test::TestNode("s", {0, 0});
        auto *tgt = new Test::TestNode("t", {300, 0});
        auto *edge = new GraphEdgeItem(src, QString(), tgt, QString(),
                                       std::make_unique<DirectPathStrategy>());
        edge->adjust();
        const QRectF before = edge->shape().boundingRect();

        tgt->setPos(300, 400);     // move the target far down
        edge->adjust();
        const QRectF after = edge->shape().boundingRect();

        QVERIFY(after != before);
        QVERIFY(after.bottom() > before.bottom() + 100.0);

        delete edge; delete src; delete tgt;
    }

    void shapeFollowsHitWidth()
    {
        using namespace Graffodil;
        auto *src = new Test::TestNode("s", {0, 0});
        auto *tgt = new Test::TestNode("t", {300, 0});
        auto *edge = new GraphEdgeItem(src, QString(), tgt, QString(),
                                       std::make_unique<DirectPathStrategy>());
        edge->adjust();
        const qreal narrow = edge->shape().boundingRect().height();
        edge->setHitWidth(40.0);
        const qreal wide = edge->shape().boundingRect().height();
        QVERIFY(wide > narrow + 10.0);

        delete edge; delete src; delete tgt;
    }
};

QTEST_MAIN(TestGraphEdgeItem)
#include "tst_graphedgeitem.moc"
