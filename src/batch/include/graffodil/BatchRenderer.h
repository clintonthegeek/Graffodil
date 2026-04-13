#pragma once

#include <QObject>

namespace Graffodil {

class GraphScene;

class BatchRenderer : public QObject {
    Q_OBJECT
public:
    explicit BatchRenderer(GraphScene *scene);
    ~BatchRenderer() override;

    void setBatchMode(bool enabled);
    bool isBatchMode() const;

    /// Auto-switch at threshold. Set 0 to disable.
    void setAutoThreshold(int nodeCount);

    /// Rebuild batch data from current scene state.
    void synchronize();

    /// LOD thresholds for progressive detail reduction.
    void setLodThresholds(qreal hideEdges, qreal hideLabels, qreal hideNodes);

private:
    class Private;
    Private *d;
};

} // namespace Graffodil
