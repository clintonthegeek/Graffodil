#include <graffodil/CircularLayout.h>
#include <graffodil/IGraphNode.h>
#include <cmath>

namespace Graffodil {

void CircularLayout::setNodes(const QList<IGraphNode *> &nodes) { m_nodes = nodes; }
void CircularLayout::setEdges(const QList<IGraphEdge *> &edges) { m_edges = edges; }
void CircularLayout::setRadius(qreal r) { m_radius = r; }
void CircularLayout::setCenterNode(const QString &id) { m_centerNodeId = id; }

LayoutResult CircularLayout::compute()
{
    LayoutResult result;

    if (m_nodes.isEmpty())
        return result;

    // Single node goes at origin
    if (m_nodes.size() == 1) {
        result.nodePositions[m_nodes.first()->nodeId()] = {0, 0};
        return result;
    }

    // Separate center node from circle nodes
    QList<IGraphNode *> circleNodes;
    for (auto *node : m_nodes) {
        if (node->nodeId() == m_centerNodeId) {
            result.nodePositions[node->nodeId()] = {0, 0};
        } else {
            circleNodes.append(node);
        }
    }

    // If no center node was set, all nodes go on circle
    if (m_centerNodeId.isEmpty())
        circleNodes = m_nodes;

    const int n = circleNodes.size();
    if (n == 0)
        return result;

    const double angleStep = 2.0 * M_PI / n;
    for (int i = 0; i < n; ++i) {
        const double angle = angleStep * i - M_PI / 2.0; // start at top
        const double x = m_radius * std::cos(angle);
        const double y = m_radius * std::sin(angle);
        result.nodePositions[circleNodes[i]->nodeId()] = {x, y};
    }

    return result;
}

} // namespace Graffodil
