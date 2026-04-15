#pragma once

#include <graffodil/GraphTool.h>
#include <graffodil/Types.h>
#include <QList>
#include <QPointF>

class QGraphicsRectItem;
class QGraphicsItemGroup;

namespace Graffodil {

class IGraphNode;
class IGraphEdge;
class IAlignmentStrategy;

/// Select and move tool.
///
/// - Plain click: clear selection, select clicked item
/// - Shift+click: add to selection (never deselects)
/// - Ctrl/Cmd+click: toggle selection of clicked item
/// - Drag on selected node: move (all selected nodes move together)
/// - Drag on empty space: rubber-band selection
/// - Delete/Backspace: emit deleteRequested
/// - R: emit reverseRequested for selected edges
class SelectMoveTool : public GraphTool {
    Q_OBJECT
public:
    using GraphTool::GraphTool;

    void activate(GraphScene *scene) override;
    void deactivate() override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    /// Install an alignment strategy consulted during drags. The tool
    /// does not take ownership. Pass nullptr to disable (default).
    void setAlignmentStrategy(IAlignmentStrategy *strategy);
    IAlignmentStrategy *alignmentStrategy() const { return m_alignment; }

signals:
    void deleteRequested(const QList<IGraphNode *> &nodes,
                         const QList<IGraphEdge *> &edges);
    void nodeMoved(IGraphNode *node, const QPointF &oldPos, const QPointF &newPos);
    void reverseRequested(const QList<IGraphEdge *> &edges);

    /// Emitted once when a drag of one or more nodes actually begins
    /// (i.e. the cursor crosses the drag threshold after a press on a
    /// node). Consumers typically use this to open an undo macro/compound.
    void dragBegan(const QList<IGraphNode *> &nodes);

    /// Emitted once after the final `nodeMoved` of a drag, on mouse
    /// release. Consumers typically use this to close the undo macro
    /// opened in dragBegan. Not emitted if the drag never started
    /// (click without crossing the threshold).
    void dragEnded(const QList<IGraphNode *> &nodes);

private:
    // Drag state
    IGraphNode *m_dragNode = nullptr;
    QList<IGraphNode *> m_draggedNodes;  // all selected nodes moving together
    QHash<IGraphNode *, QPointF> m_dragStartPositions;
    QPointF m_pressScenePos;
    bool m_dragging = false;
    IAlignmentStrategy *m_alignment = nullptr;
    QGraphicsItemGroup *m_guideOverlay = nullptr;

    // Rubber-band state
    QGraphicsRectItem *m_rubberBand = nullptr;
    QPointF m_rubberBandOrigin;
    bool m_rubberBanding = false;
    Qt::KeyboardModifiers m_rubberBandModifiers;
};

} // namespace Graffodil
