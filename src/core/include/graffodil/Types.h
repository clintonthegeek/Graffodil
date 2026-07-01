#pragma once

#include <QColor>
#include <QFont>
#include <QHash>
#include <QList>
#include <QPair>
#include <QPointF>
#include <QString>
#include <QVariant>

namespace Graffodil {

enum class ArrowEnd { Source, Target };

/// A connection point on a node's boundary (or interior).
///
/// Nodes publish a list of anchors via IGraphNode::anchors(). Edges
/// reference anchors by their `id` string, so consumers can have however
/// many they want — two (left/right for dependency graphs), four (compass
/// default), or dozens (named audio ports). An empty `id` on an edge
/// endpoint means "swivel": the edge resolves the anchor dynamically via
/// IGraphNode::anchorToward() each time it adjusts.
struct Anchor {
    /// Stable identifier within the owning node. Consumer-chosen —
    /// convention is "top"/"right"/"bottom"/"left" for compass anchors,
    /// but any string works.
    QString id;

    /// Position of the anchor in scene coordinates.
    QPointF scenePos;

    /// Unit vector pointing outward from the node at this anchor.
    /// Used by path strategies for control-point offsets and exit
    /// directions. For compass anchors this points away from the rect.
    QPointF outwardDirection;

    /// Optional consumer data (e.g. port type, label, color).
    /// Graffodil itself does not inspect this.
    QVariant metadata;
};

/// Communication contract between layout engines and the scene.
/// Layout engines produce this; the scene consumes it via applyLayout().
struct LayoutResult {
    QHash<QString, QPointF> nodePositions;
    /// Optional waypoints for edges spanning multiple layers (Sugiyama)
    /// or needing routing hints. Key is (sourceId, targetId).
    QHash<QPair<QString, QString>, QList<QPointF>> edgeWaypoints;
};

/// Phase 6c: styling for an edge's midpoint label.
struct EdgeLabelStyle {
    QFont font;
    QColor color = QColor(60, 60, 60);
    QColor background = Qt::transparent;
    qreal backgroundPadding = 2.0;
    qreal positionOnPath = 0.5;      ///< 0..1 along path (source → target)
    QPointF offset = {0.0, -6.0};    ///< px offset from the path point
    bool rotateWithPath = false;     ///< align baseline to path tangent
};

/// Lightweight descriptor for a reusable edge-midpoint affordance.
/// Consumers return these from IGraphEdge::edgeActions().  Graffodil only
/// supplies the hit-test + dispatch contract; concrete visuals and semantics
/// stay 100 % consumer-owned.
struct EdgeAction {
    QString id;      ///< stable identifier, e.g. "insert", "delete"
    qreal   pos = 0.5; ///< parametric position along the edge path [0..1]
};

} // namespace Graffodil
