# Spec: Edge Labels

**Status:** spec'd, needs implementation plan
**Depends on:** none hard; reuses cached path metrics if Phase 4 has landed
**Module:** `Graffodil::Core` (`GraphEdgeItem`)

## Problem

Nothing in the library can put text on an edge. Both consumer families need
it: PlanStan (dependency-type / lag annotations on Gantt & PERT edges) and
Corbomite (relation names on canvas links).

## Goals

- One-call label on a `GraphEdgeItem`, positioned along the path, tracking
  every `adjust()`.
- Styleable (font, color, optional background box) without inventing a
  styling framework.
- Zero cost when unused.

## Non-goals

- Labels on consumer-implemented `IGraphEdge`s (they own their own painting).
- Rich text, eliding, multi-label edges, label collision avoidance.
- Hit-testing/editing the label (it is display-only and mouse-transparent).

## API

```cpp
// New value type in include/graffodil/Types.h (or its own header):
struct EdgeLabelStyle {
    QFont font;                              // default: QFont()
    QColor color = QColor(60, 60, 60);
    QColor background = Qt::transparent;     // box behind text when not transparent
    qreal backgroundPadding = 2.0;
    qreal positionOnPath = 0.5;              // 0..1 along path (source → target)
    QPointF offset = {0.0, -6.0};            // px offset from the path point
    bool rotateWithPath = false;             // align baseline to path tangent
};

// GraphEdgeItem, public:
void setLabel(const QString &text);          // empty string removes the label
QString label() const;
void setLabelStyle(const EdgeLabelStyle &style);
EdgeLabelStyle labelStyle() const;
```

## Behavior

- The label is a child `QGraphicsItem` of the edge (so z-order, scene
  add/remove, and selection-frame exclusion come free). Implement as a small
  private child item (custom, so the background box and rotation are one
  paint), `setAcceptedMouseButtons(Qt::NoButton)`, `ItemIgnoresParentOpacity`
  not set (label dims with edge).
- Created lazily on first non-empty `setLabel`; deleted on `setLabel("")`.
- Position recomputed inside `adjust()` (and on `setLabelStyle`):
  `path().pointAtPercent(style.positionOnPath) + style.offset`, with the text
  rect centered on that point. If `rotateWithPath`, rotate by
  `path().angleAtPercent(positionOnPath)`, normalized so text is never
  upside-down (flip when angle in (90°, 270°)).
- Empty path (edge not yet adjusted): label hidden until first adjust.
- `reverse()`: positionOnPath is *not* mirrored — 0.5 stays the midpoint;
  a label at 0.2 moves to be near the new source. Document this; it matches
  "the label rides the path parameter," and consumers who care can set
  `1.0 - p` themselves.

## Testing

- Label item's scene position ≈ `pointAtPercent(0.5)` + offset after adjust;
  moves when an endpoint node moves and `adjust()` runs.
- `setLabel("")` removes the child item (childItems() count back to 0).
- Style change (offset, positionOnPath) repositions without adjust().
- Rotation flag: angle applied, and flipped in the upside-down range.
- No label ⇒ `childItems()` empty and `boundingRect()` unchanged (zero-cost
  check).
