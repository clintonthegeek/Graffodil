#pragma once

#include <graffodil/GroupStyle.h>
#include <graffodil/Types.h>
#include <QGraphicsScene>
#include <QHash>
#include <QMultiHash>
#include <QSet>
#include <QStringList>

namespace Graffodil {

class IGraphNode;
class IGraphEdge;
class GraphTool;
class GroupItem;

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

    /// Find an edge whose source is sourceId and target is targetId.
    /// Direction-sensitive; returns nullptr if only the reverse exists.
    IGraphEdge *edgeBetween(const QString &sourceId, const QString &targetId) const;

    /// Find an edge whose endpoints are {a, b} in either order. Useful
    /// for consumers treating edges as undirected, where source/target
    /// ordering is arbitrary. Returns the first match.
    IGraphEdge *edgeBetweenUndirected(const QString &a, const QString &b) const;

    QList<IGraphEdge *> edges() const;

    /// Remove all nodes and edges from the registry and scene. Returns the
    /// removed interfaces so the consumer can free their graphicsItems.
    struct ClearResult {
        QList<IGraphNode *> nodes;
        QList<IGraphEdge *> edges;
    };
    ClearResult clearGraph();

    /// Expand the scene rect to enclose all items plus a margin of
    /// `margin` scene units, giving room to pan past the content.
    /// If the scene is empty, sets a default 2*margin square centered on origin.
    void fitSceneRectToContent(qreal margin = 500.0);

    // --- Tool dispatch ---
    void setActiveTool(GraphTool *tool);
    GraphTool *activeTool() const;

    // --- Selection (typed over interfaces) ---
    QList<IGraphNode *> selectedNodes() const;
    QList<IGraphEdge *> selectedEdges() const;

    /// Id of the first registered node that is currently selected, or
    /// an empty string if none. Iteration order follows registry insertion.
    QString selectedNodeId() const;

    /// Clear the scene's current selection and select exactly the node
    /// with the given id. No-op if the id isn't registered.
    void selectNode(const QString &id);

    // --- Group regions ---
    // A "group" is a decorative backdrop drawn behind a set of member
    // nodes: a rounded, padded rect (with optional fill/border/title)
    // that tracks the members as they move. Groups never intercept
    // mouse events on their members — they are purely visual.
    //
    // Membership is addressed by node id. Removing a node cascades
    // into any group containing it. Members may overlap multiple
    // groups; draw order is controlled by GroupStyle::zValue.

    /// Register a new group. Returns a stable uuid-shaped id used
    /// in subsequent calls. The scene installs a listener on its own
    /// `changed` signal the first time a group is added, so members
    /// moved interactively or via applyLayout() keep the region
    /// tracking automatically.
    QString addGroup(const QSet<QString> &memberNodeIds,
                     const GroupStyle &style);

    void setGroupStyle(const QString &groupId, const GroupStyle &style);
    void setGroupMembers(const QString &groupId,
                         const QSet<QString> &memberNodeIds);
    void removeGroup(const QString &groupId);

    QStringList  groupIds() const;
    QSet<QString> groupMembers(const QString &groupId) const;

    /// Direct handle to the backdrop item. Exposed so consumers can
    /// style via setStyle(), or query bounds for overlays. Ownership
    /// remains with the scene. Returns nullptr for unknown id.
    GroupItem *groupItem(const QString &groupId) const;

    /// Synchronously recompute all group bounds from current member positions.
    /// Normally unnecessary — refresh is coalesced automatically once per
    /// event-loop turn — but useful right after programmatic setPos() when
    /// bounds are queried before returning to the event loop.
    void refreshGroupsNow();

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

    /// Emitted when the user double-clicks on empty scene background
    /// (no graphics item under the cursor). Useful for "double-click to create".
    void backgroundDoubleClicked(const QPointF &scenePos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) override;
    void wheelEvent(QGraphicsSceneWheelEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;

    /// Hook for subclasses that want to place a new node at an explicit
    /// scene coordinate (e.g., canvas double-click creation). The default
    /// implementation is a no-op. Consumers that need neighbor-anchored
    /// placement should continue to use their own logic around placeNewNode.
    virtual void placeNodeAt(const QPointF &scenePos);

private slots:
    void refreshGroupsFromSceneChange(const QList<QRectF> &region);

private:
    /// Recompute bounds of every group from its current members.
    /// Called after scene mutations and whenever membership changes.
    void refreshGroups();

    QHash<QString, IGraphNode *> m_nodes;
    QHash<QString, IGraphEdge *> m_edges;
    /// nodeId -> edges touching that node (both endpoints). An edge with
    /// distinct endpoints appears under two keys; a self-loop under one key.
    QMultiHash<QString, IGraphEdge *> m_edgesByNode;
    GraphTool *m_activeTool = nullptr;
    bool m_edgesAboveNodes = false;

    QHash<QString, GroupItem *>    m_groups;
    QHash<QString, QSet<QString>>  m_groupMembers;
    bool m_groupsChangeConnected = false;
    bool m_refreshingGroups = false;
    bool m_groupRefreshQueued = false;
};

} // namespace Graffodil
