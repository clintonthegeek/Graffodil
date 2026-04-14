#include <graffodil/Anchors.h>

namespace Graffodil {

QList<Anchor> compassAnchors(const QRectF &r)
{
    QList<Anchor> result;
    result.reserve(4);

    Anchor top;
    top.id = AnchorIds::Top;
    top.scenePos = {r.center().x(), r.top()};
    top.outwardDirection = {0, -1};
    result.append(top);

    Anchor right;
    right.id = AnchorIds::Right;
    right.scenePos = {r.right(), r.center().y()};
    right.outwardDirection = {1, 0};
    result.append(right);

    Anchor bottom;
    bottom.id = AnchorIds::Bottom;
    bottom.scenePos = {r.center().x(), r.bottom()};
    bottom.outwardDirection = {0, 1};
    result.append(bottom);

    Anchor left;
    left.id = AnchorIds::Left;
    left.scenePos = {r.left(), r.center().y()};
    left.outwardDirection = {-1, 0};
    result.append(left);

    return result;
}

} // namespace Graffodil
