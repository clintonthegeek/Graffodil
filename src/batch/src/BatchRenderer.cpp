#include <graffodil/BatchRenderer.h>
#include <graffodil/GraphScene.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include "BatchNodeItem.h"
#include "BatchEdgeItem.h"

namespace Graffodil {

class BatchRenderer::Private {
public:
    GraphScene *scene = nullptr;
    BatchNodeItem *nodeItem = nullptr;
    BatchEdgeItem *edgeItem = nullptr;
    bool batchMode = false;
    int autoThreshold = 0;
    qreal lodHideEdges = 0.03;
    qreal lodHideLabels = 0.1;
    qreal lodHideNodes = 0.01;
};

BatchRenderer::BatchRenderer(GraphScene *scene) : QObject(scene), d(new Private)
{
    d->scene = scene;
}

BatchRenderer::~BatchRenderer()
{
    setBatchMode(false);
    delete d;
}

void BatchRenderer::setBatchMode(bool enabled)
{
    if (d->batchMode == enabled)
        return;

    d->batchMode = enabled;

    if (enabled) {
        // Hide individual items
        for (auto *node : d->scene->nodes())
            node->graphicsItem()->setVisible(false);
        for (auto *edge : d->scene->edges())
            edge->graphicsItem()->setVisible(false);

        // Create batch items
        d->nodeItem = new BatchNodeItem();
        d->edgeItem = new BatchEdgeItem();
        d->scene->addItem(d->nodeItem);
        d->scene->addItem(d->edgeItem);

        synchronize();
    } else {
        // Remove batch items
        if (d->nodeItem) {
            d->scene->removeItem(d->nodeItem);
            delete d->nodeItem;
            d->nodeItem = nullptr;
        }
        if (d->edgeItem) {
            d->scene->removeItem(d->edgeItem);
            delete d->edgeItem;
            d->edgeItem = nullptr;
        }

        // Show individual items
        for (auto *node : d->scene->nodes())
            node->graphicsItem()->setVisible(true);
        for (auto *edge : d->scene->edges())
            edge->graphicsItem()->setVisible(true);
    }
}

bool BatchRenderer::isBatchMode() const { return d->batchMode; }

void BatchRenderer::setAutoThreshold(int nodeCount)
{
    d->autoThreshold = nodeCount;
}

void BatchRenderer::synchronize()
{
    // Auto-threshold check
    if (d->autoThreshold > 0) {
        bool shouldBatch = d->scene->nodes().size() >= d->autoThreshold;
        if (shouldBatch != d->batchMode) {
            setBatchMode(shouldBatch);
            if (!shouldBatch) return; // switched to individual mode, done
        }
    }

    if (!d->batchMode || !d->nodeItem || !d->edgeItem)
        return;

    // Build node data
    auto sceneNodes = d->scene->nodes();
    QVector<BatchNodeItem::NodeData> nodeData;
    nodeData.reserve(sceneNodes.size());
    QHash<QString, int> nodeIndexMap;

    for (int i = 0; i < sceneNodes.size(); ++i) {
        auto *node = sceneNodes[i];
        BatchNodeItem::NodeData nd;
        nd.position = node->graphicsItem()->pos();
        nd.radius = qMax(node->nodeBoundingRect().width(), node->nodeBoundingRect().height()) / 2.0;
        nd.color = QColor(100, 150, 200);
        nodeData.append(nd);
        nodeIndexMap[node->nodeId()] = i;
    }
    d->nodeItem->setNodes(nodeData);

    // Build edge data
    auto sceneEdges = d->scene->edges();
    QVector<BatchEdgeItem::EdgeData> edgeData;
    edgeData.reserve(sceneEdges.size());

    for (auto *edge : sceneEdges) {
        BatchEdgeItem::EdgeData ed;
        QPointF srcPos = edge->sourceNode()->graphicsItem()->pos();
        QPointF tgtPos = edge->targetNode()->graphicsItem()->pos();
        ed.line = QLineF(srcPos, tgtPos);
        ed.dimmed = false;
        edgeData.append(ed);
    }
    d->edgeItem->setEdges(edgeData);
}

void BatchRenderer::setLodThresholds(qreal hideEdges, qreal hideLabels, qreal hideNodes)
{
    d->lodHideEdges = hideEdges;
    d->lodHideLabels = hideLabels;
    d->lodHideNodes = hideNodes;
}

} // namespace Graffodil
