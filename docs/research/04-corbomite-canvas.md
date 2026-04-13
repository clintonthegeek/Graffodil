# Agent Report: Corbomite Canvas View

**Source:** Corbomite `libs/canvas/` (~3,851 LOC)
**Surveyed:** 2026-04-13

## Overview

Interactive canvas editor for the JSON Canvas 1.0 format (jsoncanvas.org). Spatially arranges cards connected by smooth bezier arrows. Manual placement only (no auto-layout). Matches Obsidian Canvas visual style.

## 1. Architecture

**Core Location:** `libs/canvas/`

**Primary Classes:**
- **CanvasView** (161 LOC) — QGraphicsView subclass
- **CanvasScene** (897 LOC) — QGraphicsScene managing all items and tools
- **CanvasDocument** (286 LOC) — Data model with JSON serialization
- **CanvasTypes** (84 LOC) — Type definitions and color mapping

Architecture follows PlanStan-inspired tool-based state machine (SelectMoveTool, CreateCardTool, CreateEdgeTool).

## 2. Card/Node Representation

### TextCardItem (302 LOC)
- Rectangular cards with rich markdown content
- Markdown rendering via RenderEngine (QTextDocument)
- Color-coded header bars (preset colors 1-6 or hex)
- 8-point resize handles (Kdenlive-style)
- Double-click for inline editing

### FileCardItem (219 LOC)
- References file system content with subpath support (#heading, #^block)
- Title bar with filename extraction
- Same resize/selection pattern as TextCardItem

### GroupItem (229 LOC)
- Container rectangles with transparent fill (color at 10% alpha)
- Semi-transparent borders (40% alpha or gray default)
- Bold label text at top-left
- Z-order = -1 (behind cards)

### Data Model
```cpp
struct CanvasNode {
    QString id;
    NodeType type = NodeType::Text;  // Text, File, Link, Group
    int x = 0, y = 0;
    int width = 250, height = 60;
    QString color;     // "" or "1"-"6" (presets) or "#RRGGBB"
    QString text;      // Markdown for text nodes
    QString file, subpath, url, label;
};
```

**Rendering Details:**
- Corner radius: 8.0 pixels
- Header bar: 20-32px tall
- Text padding: 8.0 pixels
- Selection: blue 2px border (RGB 58, 134, 255)

## 3. Bezier Arrow Implementation

**EdgeItem** (176 LOC) — the critical file for Graffodil.

**Technique:** QPainterPath with cubic bezier curves.

```cpp
void EdgeItem::adjust() {
    QPointF from = m_source->connectionPoint(m_data.fromSide);
    QPointF to = m_target->connectionPoint(m_data.toSide);

    double dx = to.x() - from.x();
    double dy = to.y() - from.y();
    double dist = std::sqrt(dx * dx + dy * dy);
    double curvature = std::min(dist * 0.4, 80.0);  // capped

    QPointF ctrl1 = from;
    QPointF ctrl2 = to;

    // Extend control points outward from connection side
    switch (m_data.fromSide) {
        case Side::Right:  ctrl1 += QPointF(curvature, 0); break;
        case Side::Left:   ctrl1 += QPointF(-curvature, 0); break;
        case Side::Top:    ctrl1 += QPointF(0, -curvature); break;
        case Side::Bottom: ctrl1 += QPointF(0, curvature); break;
    }
    // Same for ctrl2 with toSide

    edgePath.cubicTo(ctrl1, ctrl2, to);
}
```

**Arrowhead** (lines 85-98):
- Angle via `atan2(-line.dy(), line.dx())`
- Two wing points at ±π/6 (30°)
- Arrow size: 10.0 pixels
- Drawn as filled QPolygonF

**Parameters:**
- Pen width: 2.0 pixels default
- Curvature: `min(distance * 0.4, 80.0)` — S-curves with distance-dependent bend
- Colors: preset "1"-"6" or hex, default gray (150, 150, 150)

## 4. Connection System

**ConnectableItem interface** (20 LOC):
- `connectionPoint(Side side)` returns center of each cardinal side
- Implemented by TextCardItem, FileCardItem, GroupItem

**Edge Data:**
```cpp
struct CanvasEdge {
    QString id;
    QString fromNode, toNode;
    Side fromSide = Side::Right;
    Side toSide = Side::Left;
    EndType fromEnd = EndType::None;
    EndType toEnd = EndType::Arrow;
    QString color;
    QString label;    // optional, rendered at midpoint
};
```

## 5. Tool Implementations (CanvasTool.cpp, 606 LOC total)

### CreateEdgeTool (lines 514-604)
1. Click near card edge (within 15px threshold) → start drag
2. Find nearest cardinal side to cursor
3. Draw blue dashed preview line during drag
4. Release near another card → create edge
5. Auto-switch back to SelectMoveTool

### SelectMoveTool (lines 56-383)
- Multi-select with Shift-click
- Rubber-band selection
- Drag-to-move selected items
- 8-point resize handles (minimum 40px)
- Delete key removes selected items
- Arrow keys nudge 10px

## 6. Spatial Arrangement

**Purely manual.** No automatic layout algorithm. Users drag cards to position them. Coordinates stored as absolute (x, y) integers in JSON. Edges auto-adjust via `EdgeItem::adjust()` when cards move.

## 7. Data Model & Serialization

**Format:** JSON Canvas 1.0 (jsoncanvas.org)

```json
{
    "nodes": [
        { "id": "abc", "type": "text", "text": "**Hello**",
          "x": -280, "y": -200, "width": 250, "height": 60 }
    ],
    "edges": [
        { "id": "def", "fromNode": "abc", "toNode": "xyz",
          "fromSide": "right", "toSide": "left", "toEnd": "arrow" }
    ]
}
```

**CanvasDocument** (286 LOC): in-memory QHash<QString, CanvasNode/CanvasEdge>, modification tracking, signal-based change notifications.

## 8. Undo/Redo

Full QUndoCommand support via CanvasCommands.h:
- CmdMoveCards, CmdResizeCard
- CmdAddCard, CmdRemoveCard
- CmdAddEdge, CmdRemoveEdge
- CmdEditText, CmdChangeColor

## 9. Edge Labels

**EdgeItem.cpp lines 142-174:**
- Optional label at bezier midpoint
- White background with rounded corners
- 9pt font, centered

## 10. File Structure

```
libs/canvas/
├── include/canvas/
│   ├── CanvasView.h (43)
│   ├── CanvasScene.h (129)
│   ├── CanvasDocument.h (62)
│   ├── CanvasTypes.h (84)
│   ├── CanvasTool.h (105)
│   ├── CanvasCommands.h (130)
│   ├── TextCardItem.h (51)
│   ├── FileCardItem.h (50)
│   ├── GroupItem.h (44)
│   ├── EdgeItem.h (32)
│   └── ConnectableItem.h (20)
├── src/
│   ├── CanvasView.cpp (161)
│   ├── CanvasScene.cpp (897) — largest, event routing
│   ├── CanvasDocument.cpp (286)
│   ├── CanvasTool.cpp (606) — all tools
│   ├── TextCardItem.cpp (302)
│   ├── FileCardItem.cpp (219)
│   ├── GroupItem.cpp (229)
│   └── EdgeItem.cpp (176) — bezier curves
└── TOTAL: ~3,851 LOC
```

## 11. Reusability for Graffodil

**Directly transplantable:**
- **EdgeItem bezier curve logic** → `BezierPathStrategy` in Graffodil::Core
- **ConnectableItem interface pattern** → informed `IGraphNode::anchorPoint(Side)` design
- **CanvasTool state machine** → informed `GraphTool` base class design
- **Side enum, color mapping** → `Graffodil::Side` enum

**Stays in Corbomite:**
- TextCardItem, FileCardItem, GroupItem (domain-specific card rendering)
- CanvasDocument (JSON Canvas 1.0 serialization)
- CreateCardTool (card creation is Canvas-specific)

**Migration:** Corbomite Canvas would implement `IGraphNode` on its card items, use `GraphScene` for node/edge registry and tool dispatch, and use `GraphEdgeItem` with `BezierPathStrategy` for edges. CanvasDocument stays as the data persistence layer.
