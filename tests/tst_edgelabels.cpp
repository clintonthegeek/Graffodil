#include <QTest>
#include <QGraphicsView>

#include <graffodil/GraphScene.h>
#include <graffodil/GraphEdgeItem.h>
#include <graffodil/EdgePathStrategy.h>
#include <graffodil/Types.h>

#include "TestHelpers.h"

using namespace Graffodil;
using Graffodil::Test::TestNode;

class TstEdgeLabels : public QObject {
    Q_OBJECT
private slots:
    void labelLazilyCreated();
    void labelPositionFollowsPath();
    void emptyStringRemovesLabel();
    void styleControlsPositionOnPath();
};

namespace {
GraphEdgeItem *makeEdge(GraphScene &scene, TestNode *&a, TestNode *&b)
{
    a = new TestNode(QStringLiteral("a"), QPointF(0, 0));
    b = new TestNode(QStringLiteral("b"), QPointF(400, 0));
    scene.addNode(a);
    scene.addNode(b);
    auto *edge = new GraphEdgeItem(a, QString(), b, QString(),
                                   std::make_unique<DirectPathStrategy>());
    scene.addEdge(edge);
    edge->adjust();
    return edge;
}
} // namespace

void TstEdgeLabels::labelLazilyCreated()
{
    GraphScene scene;
    TestNode *a, *b;
    auto *edge = makeEdge(scene, a, b);

    const int before = edge->graphicsItem()->childItems().size();
    QCOMPARE(edge->label(), QString());

    edge->setLabel(QStringLiteral("42"));
    QCOMPARE(edge->label(), QStringLiteral("42"));
    QCOMPARE(edge->graphicsItem()->childItems().size(), before + 1);
}

void TstEdgeLabels::labelPositionFollowsPath()
{
    GraphScene scene;
    TestNode *a, *b;
    auto *edge = makeEdge(scene, a, b);
    edge->setLabel(QStringLiteral("mid"));
    edge->adjust();

    auto *labelItem = edge->graphicsItem()->childItems().last();
    const QPointF expected =
        edge->path().pointAtPercent(0.5) + edge->labelStyle().offset;
    // label item's scene position should sit at path midpoint + offset
    // (within a couple px — the item centers its text on that point)
    QVERIFY((labelItem->scenePos() - expected).manhattanLength() < 30.0);
}

void TstEdgeLabels::emptyStringRemovesLabel()
{
    GraphScene scene;
    TestNode *a, *b;
    auto *edge = makeEdge(scene, a, b);
    const int before = edge->graphicsItem()->childItems().size();

    edge->setLabel(QStringLiteral("x"));
    edge->setLabel(QString());
    QCOMPARE(edge->label(), QString());
    QCOMPARE(edge->graphicsItem()->childItems().size(), before);
}

void TstEdgeLabels::styleControlsPositionOnPath()
{
    GraphScene scene;
    TestNode *a, *b;
    auto *edge = makeEdge(scene, a, b);
    edge->setLabel(QStringLiteral("q"));

    EdgeLabelStyle style;
    style.positionOnPath = 0.25;
    style.offset = QPointF(0, 0);
    edge->setLabelStyle(style);
    edge->adjust();

    auto *labelItem = edge->graphicsItem()->childItems().last();
    const QPointF expected = edge->path().pointAtPercent(0.25);
    QVERIFY((labelItem->scenePos() - expected).manhattanLength() < 30.0);
}

QTEST_MAIN(TstEdgeLabels)
#include "tst_edgelabels.moc"
