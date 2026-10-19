#include <graffodil/IGraphNode.h>
#include <graffodil/Logging.h>
#include <QLineF>
#include <limits>

namespace Graffodil {

Anchor IGraphNode::anchorToward(const QPointF &other) const
{
    const QList<Anchor> list = anchors();
    if (list.isEmpty()) {
        // Silent (0,0) routed edges to the scene origin, which looked like a
        // layout bug rather than a missing-anchor bug.
        qCWarning(lcGraffodil).nospace()
            << "IGraphNode::anchorToward: node '" << nodeId()
            << "' reports no anchors. Returning a default (0,0) anchor.";
        return {};
    }
    Anchor best;
    qreal bestDist = std::numeric_limits<qreal>::max();
    for (const Anchor &a : list) {
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
    qCWarning(lcGraffodil).nospace()
        << "IGraphNode::anchor: node '" << nodeId()
        << "' has no anchor with id '" << id
        << "'. Returning a default (0,0) anchor. "
        << "(Pass an empty id to request dynamic swivel resolution.)";
    return {};
}

} // namespace Graffodil
