#include <graffodil/GraphMinimap.h>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QtTest>

using namespace Graffodil;

class TestMinimap : public QObject {
    Q_OBJECT
private slots:
    void authoredSceneRectWins()
    {
        QGraphicsScene scene;
        scene.addItem(new QGraphicsRectItem(0, 0, 100, 100));
        scene.setSceneRect(0, 0, 1000, 1000);
        QGraphicsView view(&scene);
        view.resize(200, 200);

        const QRectF r = GraphMinimap::effectiveSceneRect(&view);
        // Must reflect the authored 1000x1000 canvas, not the 100x100 items.
        QVERIFY(r.width() >= 1000.0);
        QVERIFY(r.height() >= 1000.0);
    }

    void viewportAlwaysContained()
    {
        QGraphicsScene scene;
        scene.addItem(new QGraphicsRectItem(0, 0, 100, 100));
        scene.setSceneRect(0, 0, 1000, 1000);
        QGraphicsView view(&scene);
        view.resize(200, 200);
        view.centerOn(900, 900);   // pan far from the items

        const QRectF viewport =
            view.mapToScene(view.viewport()->rect()).boundingRect();
        const QRectF r = GraphMinimap::effectiveSceneRect(&view);
        QVERIFY(r.contains(viewport));
    }

    void emptySceneYieldsNullRect()
    {
        QGraphicsScene scene;   // no items, no authored rect
        QGraphicsView view(&scene);
        QVERIFY(GraphMinimap::effectiveSceneRect(&view).isNull());
        QVERIFY(GraphMinimap::effectiveSceneRect(nullptr).isNull());
    }
};

QTEST_MAIN(TestMinimap)
#include "tst_minimap.moc"
