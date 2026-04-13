#include <QTest>
#include <graffodil/GraphScene.h>
#include <graffodil/GraphEdgeItem.h>
#include <graffodil/EdgePathStrategy.h>
#include <graffodil/TerminusStyle.h>
#include <graffodil/CircularLayout.h>
#include <graffodil/SelectMoveTool.h>
#include <graffodil/CreateEdgeTool.h>
#include <graffodil/PanZoomTool.h>
#include "TestHelpers.h"
#include <cmath>

using namespace Graffodil;
using namespace Graffodil::Test;

class TestIntegration : public QObject {
    Q_OBJECT
private slots:
    void fullWorkflow() {
        // Build a 5-node graph
        GraphScene scene;
        QList<TestNode *> nodes;
        for (int i = 0; i < 5; ++i) {
            auto *n = new TestNode(QString("n%1").arg(i));
            n->graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable);
            scene.addNode(n);
            nodes.append(n);
        }
        QCOMPARE(scene.nodes().size(), 5);

        // Add edges with bezier strategy and triangle terminus
        for (int i = 0; i < 4; ++i) {
            auto *edge = new GraphEdgeItem(
                nodes[i], Side::Right, nodes[i + 1], Side::Left,
                new BezierPathStrategy);
            edge->setTerminus(ArrowEnd::Target, std::make_unique<TriangleTerminus>());
            scene.addEdge(edge);
        }
        QCOMPARE(scene.edges().size(), 4);

        // Apply circular layout
        CircularLayout layout;
        layout.setRadius(200);
        layout.setNodes(scene.nodes());
        layout.setEdges(scene.edges());
        auto result = layout.compute();
        scene.applyLayout(result);

        // Verify no nodes at origin (all on circle)
        for (auto *node : scene.nodes()) {
            QPointF pos = node->graphicsItem()->pos();
            double dist = std::sqrt(pos.x() * pos.x() + pos.y() * pos.y());
            QVERIFY2(dist > 50.0,
                qPrintable(QString("Node %1 too close to origin: %2")
                    .arg(node->nodeId()).arg(dist)));
        }

        // Adjust edges after layout
        for (auto *edge : scene.edges()) {
            auto *edgeItem = dynamic_cast<GraphEdgeItem *>(edge);
            if (edgeItem)
                edgeItem->adjust();
        }

        // Verify edges have non-empty paths
        for (auto *edge : scene.edges()) {
            auto *edgeItem = dynamic_cast<GraphEdgeItem *>(edge);
            if (edgeItem)
                QVERIFY(!edgeItem->path().isEmpty());
        }

        // Activate select tool
        SelectMoveTool selectTool;
        scene.setActiveTool(&selectTool);
        QCOMPARE(scene.activeTool(), &selectTool);

        // Switch to create edge tool
        CreateEdgeTool edgeTool;
        scene.setActiveTool(&edgeTool);
        QCOMPARE(scene.activeTool(), &edgeTool);

        // Switch to pan/zoom
        PanZoomTool panTool;
        scene.setActiveTool(&panTool);
        QCOMPARE(scene.activeTool(), &panTool);
    }
};

QTEST_MAIN(TestIntegration)
#include "tst_integration.moc"
