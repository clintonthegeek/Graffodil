#pragma once

#include <graffodil/LayoutEngine.h>

namespace Graffodil {

class CircularLayout : public LayoutEngine {
public:
    void setNodes(const QList<IGraphNode *> &nodes) override;
    void setEdges(const QList<IGraphEdge *> &edges) override;

    LayoutResult compute() override;
    bool isIterative() const override { return false; }

    void setRadius(qreal r);
    void setCenterNode(const QString &id);

private:
    QList<IGraphNode *> m_nodes;
    QList<IGraphEdge *> m_edges;
    qreal m_radius = 100.0;
    QString m_centerNodeId;
};

} // namespace Graffodil
