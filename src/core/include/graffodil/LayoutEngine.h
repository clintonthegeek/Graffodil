#pragma once

#include <graffodil/Types.h>
#include <QList>

namespace Graffodil {

class IGraphNode;
class IGraphEdge;

/// Base class for layout algorithms. Produces a LayoutResult
/// that the consumer applies to the scene via scene->applyLayout().
/// The engine does not touch scene items directly.
class LayoutEngine {
public:
    virtual ~LayoutEngine() = default;

    virtual void setNodes(const QList<IGraphNode *> &nodes) = 0;
    virtual void setEdges(const QList<IGraphEdge *> &edges) = 0;

    /// Compute positions in one shot. For iterative layouts, runs
    /// the full simulation to convergence.
    virtual LayoutResult compute() = 0;

    /// For iterative layouts: run one simulation step.
    /// Returns false when converged.
    virtual bool tick() { return false; }

    /// Whether this engine is iterative (force-directed) or one-shot (Sugiyama).
    virtual bool isIterative() const { return false; }

    /// Current positions after the last tick(). Only meaningful for
    /// iterative layouts. One-shot layouts return empty until compute().
    virtual LayoutResult currentPositions() const { return {}; }
};

} // namespace Graffodil
