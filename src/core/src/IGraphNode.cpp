#include <graffodil/IGraphNode.h>
#include <QLineF>
#include <limits>

namespace Graffodil {

Anchor IGraphNode::anchorToward(const QPointF &other) const
{
    Anchor best;
    qreal bestDist = std::numeric_limits<qreal>::max();
    for (const Anchor &a : anchors()) {
        const qreal d = QLineF(a.scenePos, other).length();
        if (d < bestDist) {
            bestDist = d;
            best = a;
        }
    }
    return best;
}

Anchor IGraphNode::anchor(const QString &id) const
{
    for (const Anchor &a : anchors()) {
        if (a.id == id)
            return a;
    }
    return {};
}

} // namespace Graffodil
