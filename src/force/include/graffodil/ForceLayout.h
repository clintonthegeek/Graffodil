#pragma once

#include <graffodil/LayoutEngine.h>
#include <QHash>
#include <QVector>
#include <QPointF>

namespace Graffodil {

class ForceLayout : public LayoutEngine {
public:
    void setNodes(const QList<IGraphNode *> &nodes) override;
    void setEdges(const QList<IGraphEdge *> &edges) override;

    /// Full simulation to convergence (uses multilevel optimization for initial positions)
    LayoutResult compute() override;

    /// Run one simulation step. Returns false when converged.
    bool tick() override;

    bool isIterative() const override { return true; }
    bool isConverged() const;

    LayoutResult currentPositions() const override;

    void setCenterForce(qreal f);
    void setRepelForce(qreal f);
    void setLinkForce(qreal f);
    void setLinkDistance(qreal d);

    /// Pin a node in place (e.g., user dragged it during simulation)
    void pinNode(const QString &id, const QPointF &pos);
    void unpinNode(const QString &id);

private:
    void initialize();
    void buildInternalStructures();
    double estimateCanvasArea() const;

    // Input
    QList<IGraphNode *> m_inputNodes;
    QList<IGraphEdge *> m_inputEdges;

    // Internal arrays (integer-indexed for performance)
    QVector<QPointF> m_positions;
    QVector<QString> m_ids;            // index -> nodeId
    QHash<QString, int> m_idToIndex;   // nodeId -> index
    struct EdgeData { int src; int tgt; };
    QVector<EdgeData> m_edges;
    QVector<bool> m_pinned;
    QHash<QString, int> m_degree;

    // Simulation state
    QVector<QPointF> m_displacements;
    QVector<QPointF> m_previousForces;
    double m_globalSpeed = 1.0;
    double m_energy = 0.0;
    double m_prevEnergy = 0.0;
    int m_iteration = 0;
    int m_stableCount = 0;
    int m_energyStableCount = 0;
    bool m_converged = false;
    bool m_initialized = false;

    // Deferred pin requests (applied during initialize())
    QHash<QString, QPointF> m_pendingPins;

    // Parameters
    double m_centerForce = 0.01;
    double m_repelForce = 1500.0;
    double m_linkForce = 0.05;
    double m_linkDistance = 100.0;

    static constexpr double EPSILON = 0.001;
    static constexpr int BARNES_HUT_THRESHOLD = 100;
    static constexpr int STABLE_ITERATIONS_REQUIRED = 5;
};

} // namespace Graffodil
