#pragma once

#include <QColor>
#include <QObject>

#include <functional>

namespace Graffodil {

class GraphScene;
class IGraphNode;

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

    /// Colour provider for batch-mode node discs. When set, called once
    /// per registered node during synchronize(); when unset, the scene's
    /// Palette::batchNode is used (theme it via GraphScene::setPalette).
    /// A returning QColor with alpha 0 keeps a node from drawing a disc.
    void setNodeColorProvider(std::function<QColor(const IGraphNode *)> provider);

private:
    class Private;
    Private *d;
};

} // namespace Graffodil
