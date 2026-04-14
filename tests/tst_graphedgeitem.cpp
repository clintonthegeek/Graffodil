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
        auto *strategy = new DirectPathStrategy;
        GraphEdgeItem edge(&src, AnchorIds::Right, &tgt, AnchorIds::Left, strategy);

        QVERIFY(!edge.edgeId().isEmpty());
        QCOMPARE(edge.sourceNode(), static_cast<IGraphNode *>(&src));
        QCOMPARE(edge.targetNode(), static_cast<IGraphNode *>(&tgt));
        QVERIFY(edge.graphicsItem() != nullptr);
    }

    void adjustUpdatesPath() {
        TestNode src("src", {0, 0}), tgt("tgt", {200, 0});
        auto *strategy = new DirectPathStrategy;
        GraphEdgeItem edge(&src, AnchorIds::Right, &tgt, AnchorIds::Left, strategy);

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
        auto *direct = new DirectPathStrategy;
        GraphEdgeItem edge(&src, AnchorIds::Right, &tgt, AnchorIds::Left, direct);
        edge.adjust();
        int directElements = edge.path().elementCount();

        auto *bezier = new BezierPathStrategy;
        edge.setPathStrategy(bezier);
        edge.adjust();
        int bezierElements = edge.path().elementCount();

        QVERIFY(bezierElements > directElements);
    }

    void terminusOwnership() {
        TestNode src("src", {0, 0}), tgt("tgt", {200, 0});
        auto *strategy = new DirectPathStrategy;
        GraphEdgeItem edge(&src, AnchorIds::Right, &tgt, AnchorIds::Left, strategy);

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

        auto *strategy = new DirectPathStrategy;
        auto *edge = new GraphEdgeItem(src, AnchorIds::Right, tgt, AnchorIds::Left, strategy);
        scene.addEdge(edge);

        QCOMPARE(scene.edges().size(), 1);
        QCOMPARE(scene.edgeBetween("src", "tgt"), static_cast<IGraphEdge *>(edge));
    }
};

QTEST_MAIN(TestGraphEdgeItem)
#include "tst_graphedgeitem.moc"
