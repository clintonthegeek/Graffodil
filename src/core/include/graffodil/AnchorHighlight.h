#pragma once

#include <graffodil/Types.h>
#include <QGraphicsItem>

namespace Graffodil {

class IGraphNode;

class AnchorHighlight : public QGraphicsItem {
public:
    AnchorHighlight();

    void showAnchors(IGraphNode *node);
    void hideAnchors();
    void setActiveAnchor(Side side);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

private:
    IGraphNode *m_node = nullptr;
    Side m_activeSide = Side::Right;
    bool m_visible = false;
    static constexpr qreal kDotRadius = 5.0;
};

} // namespace Graffodil
