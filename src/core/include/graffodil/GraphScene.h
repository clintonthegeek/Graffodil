#pragma once

#include <graffodil/Types.h>
#include <QGraphicsScene>
#include <QHash>

namespace Graffodil {

class IGraphNode;
class IGraphEdge;
class GraphTool;

class GraphScene : public QGraphicsScene {
    Q_OBJECT
public:
    using QGraphicsScene::QGraphicsScene;

    // --- Node/edge registry ---
    void addNode(IGraphNode *node);
    void removeNode(const QString &id);
    IGraphNode *nodeForId(const QString &id) const;
    QList<IGraphNode *> nodes() const;

    void addEdge(IGraphEdge *edge);
    void removeEdge(const QString &id);
    IGraphEdge *edgeForId(const QString &id) const;
    IGraphEdge *edgeBetween(const QString &sourceId, const QString &targetId) const;
    QList<IGraphEdge *> edges() const;

    // --- Tool dispatch ---
    void setActiveTool(GraphTool *tool);
    GraphTool *activeTool() const;

    // --- Selection (typed over interfaces) ---
    QList<IGraphNode *> selectedNodes() const;
    QList<IGraphEdge *> selectedEdges() const;

    // --- Layout ---
    void applyLayout(const LayoutResult &result);

    /// Recompute paths for all edges connected to the given node.
    /// Call this after moving a node so edges follow correctly.
    /// The built-in SelectMoveTool and Force layout call this automatically.
    void adjustEdgesForNode(const QString &nodeId);

    /// Recompute paths for all edges in the scene.
    void adjustAllEdges();

signals:
    void nodeAdded(IGraphNode *node);
    void nodeRemoved(const QString &id);
    void edgeAdded(IGraphEdge *edge);
    void edgeRemoved(const QString &id);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    QHash<QString, IGraphNode *> m_nodes;
    QHash<QString, IGraphEdge *> m_edges;
    GraphTool *m_activeTool = nullptr;
};

} // namespace Graffodil
