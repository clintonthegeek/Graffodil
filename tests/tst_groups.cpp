#include <QTest>
#include "TestHelpers.h"
#include <graffodil/GraphScene.h>
#include <graffodil/GroupItem.h>
#include <graffodil/GroupStyle.h>

using namespace Graffodil;
using namespace Graffodil::Test;

class TestGroupRegions : public QObject {
    Q_OBJECT
private slots:
    void addGroupReturnsStableId() {
        GraphScene scene;
        auto *n1 = new TestNode("n1");
        scene.addNode(n1);

        GroupStyle style;
        style.padding = 10.0;
        const QString gid = scene.addGroup({"n1"}, style);

        QVERIFY(!gid.isEmpty());
        QVERIFY(scene.groupIds().contains(gid));
        QCOMPARE(scene.groupMembers(gid), QSet<QString>{ "n1" });
        QVERIFY(scene.groupItem(gid) != nullptr);
    }

    void boundsEncloseMembersWithPadding() {
        // Single node at (50, 50) with size 100x60 and padding 10
        // should produce an outer rect at (40, 40, 120, 80).
        GraphScene scene;
        auto *n = new TestNode("n", {50, 50});
        scene.addNode(n);

        GroupStyle style;
        style.padding = 10.0;
        const QString gid = scene.addGroup({"n"}, style);

        const QRectF outer = scene.groupItem(gid)->bounds();
        QCOMPARE(outer, QRectF(40, 40, 120, 80));
    }

    void boundsFollowLayoutMove() {
        GraphScene scene;
        auto *n = new TestNode("n", {0, 0});
        scene.addNode(n);

        GroupStyle style;
        style.padding = 5.0;
        const QString gid = scene.addGroup({"n"}, style);

        LayoutResult lr;
        lr.nodePositions["n"] = {200, 300};
        scene.applyLayout(lr);

        const QRectF outer = scene.groupItem(gid)->bounds();
        // Node rect top-left now (200,300), size 100x60; padded by 5.
        QCOMPARE(outer, QRectF(195, 295, 110, 70));
    }

    void setGroupMembersRecomputesBounds() {
        GraphScene scene;
        auto *a = new TestNode("a", {0, 0});
        auto *b = new TestNode("b", {300, 0});
        scene.addNode(a);
        scene.addNode(b);

        GroupStyle style;
        style.padding = 0.0;
        const QString gid = scene.addGroup({"a"}, style);

        QRectF before = scene.groupItem(gid)->bounds();
        QCOMPARE(before, QRectF(0, 0, 100, 60));

        scene.setGroupMembers(gid, {"a", "b"});
        QRectF after = scene.groupItem(gid)->bounds();
        QCOMPARE(after, QRectF(0, 0, 400, 60));
    }

    void removeNodeCascadesIntoGroupMembership() {
        GraphScene scene;
        auto *a = new TestNode("a");
        auto *b = new TestNode("b", {200, 0});
        scene.addNode(a);
        scene.addNode(b);

        GroupStyle style;
        const QString gid = scene.addGroup({"a", "b"}, style);
        QCOMPARE(scene.groupMembers(gid).size(), 2);

        scene.removeNode("a");
        QCOMPARE(scene.groupMembers(gid).size(), 1);
        QVERIFY(!scene.groupMembers(gid).contains("a"));
        QVERIFY(scene.groupMembers(gid).contains("b"));

        delete a; // scene doesn't own nodes
    }

    void removeGroupDropsTheItem() {
        GraphScene scene;
        auto *n = new TestNode("n");
        scene.addNode(n);

        const QString gid = scene.addGroup({"n"}, {});
        QCOMPARE(scene.groupIds().size(), 1);
        scene.removeGroup(gid);
        QCOMPARE(scene.groupIds().size(), 0);
        QVERIFY(scene.groupItem(gid) == nullptr);
    }

    void groupShapeIsEmptyForClickThrough() {
        // The group never participates in hit testing.
        GraphScene scene;
        auto *n = new TestNode("n");
        scene.addNode(n);

        const QString gid = scene.addGroup({"n"}, {});
        GroupItem *g = scene.groupItem(gid);
        QVERIFY(g != nullptr);
        QVERIFY(g->shape().isEmpty());
    }

    void membersMayOverlapMultipleGroups() {
        // A node belonging to two groups contributes to both bounds.
        GraphScene scene;
        auto *n = new TestNode("n", {10, 10});
        scene.addNode(n);

        GroupStyle s1; s1.padding = 0;
        GroupStyle s2; s2.padding = 5;
        const QString g1 = scene.addGroup({"n"}, s1);
        const QString g2 = scene.addGroup({"n"}, s2);

        QCOMPARE(scene.groupItem(g1)->bounds(), QRectF(10, 10, 100, 60));
        QCOMPARE(scene.groupItem(g2)->bounds(), QRectF(5, 5, 110, 70));
    }

    void groupZValueMatchesStyle() {
        GraphScene scene;
        auto *n = new TestNode("n");
        scene.addNode(n);

        GroupStyle style;
        style.zValue = -7.5;
        const QString gid = scene.addGroup({"n"}, style);

        QCOMPARE(scene.groupItem(gid)->zValue(), -7.5);
    }

    void clearGraphRemovesGroups() {
        GraphScene scene;
        auto *n = new TestNode("n");
        scene.addNode(n);
        const QString gid = scene.addGroup({"n"}, {});
        QVERIFY(scene.groupItem(gid) != nullptr);

        scene.clearGraph();
        QCOMPARE(scene.groupIds().size(), 0);

        delete n;
    }
};

QTEST_MAIN(TestGroupRegions)
#include "tst_groups.moc"
