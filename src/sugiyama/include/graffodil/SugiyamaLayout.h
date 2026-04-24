#pragma once

#include <graffodil/LayoutEngine.h>
#include <QSizeF>
#include <QStringList>

namespace Graffodil {

/// Five-phase Sugiyama layered layout engine.
/// Transplanted from PlanStan libs/graph/src/graphlayout.cpp.
///
/// Phases:
/// 1. Connected component detection (BFS, deterministic ordering)
/// 2. Longest-path layer assignment with vertex promotion
/// 3. Dummy vertex insertion for multi-layer edges
/// 4. Median-based crossing minimization (adaptive, up to 24 sweeps)
/// 5. Brandes-Kopf compact coordinate assignment
class SugiyamaLayout : public LayoutEngine {
public:
    /// Orientation of the layered output. Source nodes live on the
    /// side opposite their successors; arrowheads land on the
    /// successor side. `setSpacing(horizontal, vertical)` remains
    /// defined in final output-space axes regardless of direction.
    enum class Direction {
        LeftToRight, ///< Default. Layer axis = +X, within-layer axis = Y.
        RightToLeft, ///< Mirror of LeftToRight about the layout's X-center.
        TopToBottom, ///< Layer axis = +Y, within-layer axis = X.
        BottomToTop  ///< Mirror of TopToBottom about the layout's Y-center.
    };

    void setNodes(const QList<IGraphNode *> &nodes) override;
    void setEdges(const QList<IGraphEdge *> &edges) override;

    LayoutResult compute() override;
    bool isIterative() const override { return false; }

    void setNodeSize(const QSizeF &size);
    void setSpacing(qreal horizontal, qreal vertical);

    /// Orientation of the produced layout. Defaults to LeftToRight,
    /// matching the engine's historical behavior.
    void setDirection(Direction d);
    Direction direction() const;

    /// Optional: ordered list of node IDs to center-align as a visual spine.
    void setSpine(const QStringList &orderedNodeIds);
    void setSpineAlignment(bool enabled);

private:
    QList<IGraphNode *> m_nodes;
    QList<IGraphEdge *> m_edges;
    QSizeF m_nodeSize = {100, 60};
    qreal m_hSpacing = 60;
    qreal m_vSpacing = 40;
    QStringList m_spine;
    bool m_spineAlignment = false;
    Direction m_direction = Direction::LeftToRight;
};

} // namespace Graffodil
