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

    /// Default z-values. Consumers can override per-item by calling
    /// setZValue() themselves; the scene only assigns these to items
    /// still at the Qt default of 0.
    static constexpr qreal DefaultNodeZ = 0.0;
    static constexpr qreal DefaultEdgeZ = -1.0;  // below nodes

    /// Flip the default z-order policy: edges rendered above nodes.
    /// Affects subsequently added items and re-assigns existing ones
    /// that still match the prior default.
    void setEdgesAboveNodes(bool above);
    bool edgesAboveNodes() const;

    // --- Node/edge registry ---
    void addNode(IGraphNode *node);

    /// Remove the node from the registry AND remove any edges referencing it.
    /// The consumer still owns the QGraphicsItems — query connectedEdges()
    /// first if they need to free them. The scene never deletes items.
    void removeNode(const QString &id);

    /// All edges with the given node as source or target.
    QList<IGraphEdge *> connectedEdges(const QString &nodeId) const;

    IGraphNode *nodeForId(const QString &id) const;
    QList<IGraphNode *> nodes() const;

    void addEdge(IGraphEdge *edge);
    void removeEdge(const QString &id);
    IGraphEdge *edgeForId(const QString &id) const;
    IGraphEdge *edgeBetween(const QString &sourceId, const QString &targetId) const;
    QList<IGraphEdge *> edges() const;

    /// Remove all nodes and edges from the registry and scene. Returns the
    /// removed interfaces so the consumer can free their graphicsItems.
    struct ClearResult {
        QList<IGraphNode *> nodes;
        QList<IGraphEdge *> edges;
    };
    ClearResult clearGraph();

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
    void wheelEvent(QGraphicsSceneWheelEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    QHash<QString, IGraphNode *> m_nodes;
    QHash<QString, IGraphEdge *> m_edges;
    GraphTool *m_activeTool = nullptr;
    bool m_edgesAboveNodes = false;
};

} // namespace Graffodil
