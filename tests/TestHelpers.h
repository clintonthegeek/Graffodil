#pragma once

#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include <graffodil/Anchors.h>
#include <QGraphicsRectItem>

namespace Graffodil::Test {

/// Minimal IGraphNode for testing. A 100x60 rectangle with 4 compass anchors.
class TestNode : public QGraphicsRectItem, public IGraphNode {
public:
    explicit TestNode(const QString &id = {}, const QPointF &pos = {})
        : QGraphicsRectItem(0, 0, 100, 60), m_id(id)
    {
        setPos(pos);
    }

    QString nodeId() const override { return m_id; }

    QList<Anchor> anchors() const override
    {
        return compassAnchors(mapToScene(rect()).boundingRect());
    }

    QRectF nodeBoundingRect() const override { return mapToScene(rect()).boundingRect(); }
    QGraphicsItem *graphicsItem() override { return this; }

private:
    QString m_id;
};

/// Minimal IGraphEdge for testing.
class TestEdge : public QGraphicsLineItem, public IGraphEdge {
public:
    TestEdge(const QString &id, IGraphNode *source, IGraphNode *target)
        : m_id(id), m_source(source), m_target(target) {}

    QString edgeId() const override { return m_id; }
    IGraphNode *sourceNode() const override { return m_source; }
    IGraphNode *targetNode() const override { return m_target; }
    QGraphicsItem *graphicsItem() override { return this; }

private:
    QString m_id;
    IGraphNode *m_source;
    IGraphNode *m_target;
};

} // namespace Graffodil::Test
