#pragma once

#include <QColor>

namespace Graffodil {

/// The colours Graffodil itself paints (selection highlight, tool
/// feedback, minimap chrome, batch-mode fallback rendering). Nodes and
/// edge *content* are consumer-drawn and are not covered — consumers
/// already own that surface via their QGraphicsItems.
///
/// Defaults reproduce the previous hard-coded values, so a scene with a
/// default Palette is pixel-identical to pre-palette Graffodil.
///
/// Threading model:
///  - Set it on the scene with GraphScene::setPalette(). Newly added
///    edges pick it up automatically; registered GraphEdgeItems get the
///    selection colour pushed on change.
///  - Tools and the minimap read from the scene at gesture/paint time,
///    so a later setPalette() applies without re-creating anything.
///  - Batch mode reads from the scene on every synchronize(); see
///    BatchRenderer::setNodeColorProvider() for per-node colouring.
struct Palette {
    // --- Interaction highlights ---
    /// Stroke colour for selected edges (GraphEdgeItem selection).
    QColor edgeSelection = QColor(0, 120, 215);
    /// Active anchor dot in the CreateEdgeTool anchor highlight.
    QColor anchorDot = QColor(0, 120, 215);
    /// Interior fill of inactive anchor dots; active dots keep the edge
    /// highlight colour ring. Default reproduces the original Qt::white.
    QColor anchorDotFill = Qt::white;
    /// CreateEdgeTool's dashed preview line.
    QColor connectPreview = QColor(0, 120, 215);
    /// SelectMoveTool rubber band stroke.
    QColor rubberBand = QColor(0, 120, 215);
    /// SelectMoveTool rubber band translucent fill.
    QColor rubberBandFill = QColor(0, 120, 215, 30);
    /// SelectMoveTool alignment guide lines.
    QColor guideLine = QColor(255, 100, 200);

    // --- Minimap chrome ---
    QColor minimapBackground = QColor(30, 30, 30, 200);
    QColor minimapBorder     = QColor(100, 100, 100);
    QColor minimapViewRect   = QColor(0, 150, 255);

    // --- Batch mode ---
    QColor batchNode      = QColor(100, 150, 200);
    QColor batchEdgeNormal = QColor(150, 150, 150, 60);
    QColor batchEdgeDimmed = QColor(200, 200, 200, 20);
};

} // namespace Graffodil
