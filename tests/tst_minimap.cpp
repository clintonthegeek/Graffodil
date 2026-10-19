#include <graffodil/GraphMinimap.h>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QtTest>
#include <QScrollBar>

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

    // ASK-punch list: there was no mouseReleaseEvent, so one click latched
    // m_dragging: every later hover re-centred the view with no button held.
    void dragEndsOnRelease()
    {
        QGraphicsScene scene;
        scene.addItem(new QGraphicsRectItem(0, 0, 100, 100));
        scene.setSceneRect(0, 0, 1000, 1000);
        QGraphicsView view(&scene);
        view.resize(200, 200);

        GraphMinimap minimap(&view);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        QMouseEvent press(QEvent::MouseButtonPress, QPointF(10, 10), QPointF(10, 10),
                          QPointF(10, 10), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(&minimap, &press);

        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(10, 10), QPointF(10, 10),
                            QPointF(10, 10), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(&minimap, &release);

        const int before = view.horizontalScrollBar()->value();
        QMouseEvent hover(QEvent::MouseMove, QPointF(120, 80), QPointF(120, 80),
                          QPointF(120, 80), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(&minimap, &hover);

        QCOMPARE(view.horizontalScrollBar()->value(), before);
    }

    // A zero-extent scene rect must not be scaled by infinity.
    void degenerateSceneRectPaintsSafely()
    {
        QGraphicsScene scene;
        scene.setSceneRect(0, 0, 0, 0);
        QGraphicsView view(&scene);
        view.resize(200, 200);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        GraphMinimap minimap(&view);
        minimap.show();
        QVERIFY(QTest::qWaitForWindowExposed(&minimap));
        minimap.repaint();   // must not divide by a zero extent
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
