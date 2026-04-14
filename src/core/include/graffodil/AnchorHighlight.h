#pragma once

#include <graffodil/Types.h>
#include <QGraphicsItem>
#include <QString>

namespace Graffodil {

class IGraphNode;

/// Visual feedback for which anchor of a node the cursor is closest to.
/// Draws a dot for each of the node's anchors; the "active" anchor (id
/// matches m_activeAnchorId) is rendered more prominently.
class AnchorHighlight : public QGraphicsItem {
public:
    AnchorHighlight();

    void showAnchors(IGraphNode *node);
    void hideAnchors();

    /// Highlight the anchor with this id (if the current node has one
    /// matching). Pass an empty string for "no active anchor".
    void setActiveAnchor(const QString &anchorId);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

private:
    IGraphNode *m_node = nullptr;
    QString m_activeAnchorId;
    bool m_visible = false;
    static constexpr qreal kDotRadius = 5.0;
};

} // namespace Graffodil
