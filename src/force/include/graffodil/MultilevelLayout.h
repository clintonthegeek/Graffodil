#pragma once

#include <QVector>
#include <QPointF>

namespace Graffodil {

struct MultilevelConfig {
    double repelForce = 1500.0;
    double linkForce = 0.05;
    double linkDistance = 100.0;
    double centerForce = 0.01;
    int minCoarseNodes = 50;
    int coarsestIterations = 200;
    double convergenceThreshold = 0.5;
    double refinementConvergenceThreshold = 15.0;
};

class MultilevelLayout {
public:
    struct Level {
        int nodeCount = 0;
        QVector<int> edgeSrc;
        QVector<int> edgeTgt;
        QVector<double> edgeWeight;
        QVector<double> nodeWeight;
        QVector<int> fineToCoarse;
        QVector<QPointF> positions;
    };

    /// Compute good initial positions via coarsen -> layout -> uncoarsen.
    /// Input: positions and edge arrays (integer-indexed).
    /// Returns optimized positions.
    static QVector<QPointF> computeLayout(
        const QVector<QPointF> &positions,
        const QVector<int> &edgeSrc,
        const QVector<int> &edgeTgt,
        const MultilevelConfig &config = MultilevelConfig{});

    static void layoutLevel(Level &level, const MultilevelConfig &config, int iterations);

private:
    static Level buildLevel0(const QVector<QPointF> &positions,
                             const QVector<int> &edgeSrc,
                             const QVector<int> &edgeTgt);
    static Level coarsen(const Level &fine);
    static void interpolate(Level &fine, const Level &coarse);
};

} // namespace Graffodil
