# Agent Report: Corbomite Force-Directed Graph View

**Source:** Corbomite `libs/forcegraph/` (~3,283 LOC)
**Surveyed:** 2026-04-13

## Overview

Force-directed graph visualizing all markdown files as nodes with wiki-link connections. Optimized for thousands of nodes with batch rendering, spatial acceleration, and multilevel layout.

## 1. Main Graph View Widget

**File:** `libs/forcegraph/include/forcegraph/ForceGraphView.h`
**Class:** `ForceGraph::ForceGraphView : QGraphicsView`
**Combined LOC:** ~260

- Owns ForceGraphScene and connects to ForceLayoutEngine
- Viewport update mode switches: FullViewportUpdate during simulation, MinimalViewportUpdate when stable
- Edge hiding threshold at 2000 nodes during interaction (150ms debounce)

**Key Methods:** `setEngine()`, `setNodes()`/`setEdges()`, `zoomToFit()`, `zoomToNode()` (animated with QTimeLine), `setSearchFilter()`

## 2. Node Representation

**Class:** `ForceGraph::ForceGraphNode : QGraphicsEllipseItem` (~30 LOC)

```cpp
struct GraphNode {
    QString id;            // Relative file path
    QString label;         // Display name
    QString tooltip;       // Rich HTML with link count & tags
    double radius;         // 4 + log(1+degree)*3
    QColor color;          // Theme-aware, based on type
    QPointF position;
    int degree;
    NodeType type;         // Regular, Hub(6+), Orphan(0), Unresolved, DailyNote
    bool pinned;
};
```

**Node Classification:**
- Hub: degree >= 6, bright purple
- Regular: 1-5 links, standard purple
- Orphan: 0 links, gray
- Unresolved: target doesn't exist, darker gray
- DailyNote: YYYY-MM-DD filename, teal

**Radius formula:** `4.0 + log(1.0 + degree) * 3.0`

## 3. Edge/Link Representation

**Class:** `ForceGraph::ForceGraphEdge : QGraphicsLineItem` (~30 LOC)

```cpp
struct GraphEdge {
    QString sourceId;
    QString targetId;
};
```

Simple directed edges, straight lines. Optional arrowheads (toggled, suppressed at LOD < 0.5). Width scalable 0.5-3.0x. Embed links excluded from graph.

## 4. Layout Algorithm: Modified ForceAtlas2

**File:** `libs/forcegraph/src/ForceLayoutEngine.cpp` (~600 LOC)

**Core Forces:**
1. **Repulsive** (inverse square): f_r = repelForce / d². Weighted by √(deg_i * deg_j). Barnes-Hut approximation for n > 100.
2. **Attractive** (spring): f_a = (d - linkDistance) * linkForce. Applied along edges only.
3. **Center**: gravity = position * centerForce * degree.

**Adaptive Speed Control (ForceAtlas2):**
- Per-node swinging/traction tracking
- Global speed = tolerance * globalTraction / globalSwinging
- Per-node local speed = globalSpeed / (1 + globalSpeed * √swinging)

**Convergence Detection:**
- Energy-based: stops when energy decreases < 0.1% for 10 iterations
- Displacement-based: stops when max displacement < 0.01% of canvas area for 5 iterations
- Adaptive theta: 1.2 (fast) → 0.8 (balanced) → 0.5 (precise)

**Parameters (user-tunable):**
- centerForce: 0.0–0.05, default 0.01
- repelForce: 0–5000, default 1500
- linkForce: 0.0–0.2, default 0.05
- linkDistance: 20–300, default 100

## 5. Multilevel Layout Optimization

**File:** `libs/forcegraph/src/MultilevelLayout.cpp` (~250 LOC)

**Pipeline:**
1. buildLevel0() — string-keyed → integer-indexed
2. coarsen() (recursive) — Walshaw degree-ordered maximal edge matching
3. layoutLevel() — force-directed at each level
4. interpolate() (recursive) — uncoarsen, project positions

**Coarsening:** 0.5-0.7 ratio per level, 3-5 hierarchy levels

**Iteration Budgets:**
- Coarsest: 200 iterations
- Refinement: max(20, √n * 1.5)
- Early convergence when displacement < 0.5 for 3 iterations

**Performance:** 5K nodes: 4.6s. 10K nodes: 8.5s (grid) to 20s (scale-free).

## 6. Spatial Acceleration

**QuadTree** (`QuadTree.h`, 36 LOC header):
- Barnes-Hut for n > 100 (BARNES_HUT_THRESHOLD)
- Max depth 30
- Adaptive theta based on energy

**SpatialGrid** (`SpatialGrid.h`, 97 LOC, header-only):
- Fixed 64x64 grid
- Prefix-sum packed storage, O(1) cell lookup
- Rebuilt every 10 position updates
- Used by BatchNodeItem for viewport culling

## 7. Batch Rendering

**BatchNodeItem** / **BatchEdgeItem**:
- Single paint call for all nodes/edges
- LOD tiers: < 0.03 skip, 0.03-0.05 2x2 rect, 0.05-0.3 filled rect, > 0.3 full circle + label
- Text greeking: gray rectangle below fade threshold
- Cosmetic pens (1 device pixel, not scaled)
- Pre-sorted edges (normal first, dimmed at end) to minimize pen changes

**Mode Switching:**
- During simulation: batch items visible, individual items hidden
- When stable: individual items visible for interaction

## 8. Data Model & Parsing

**GraphDataBuilder** (app layer, not in library):
- Sources: SQLiteIndex (link queries) + VaultModel (file metadata)
- Link types: "wiki" (`[[Note]]`), "markdown" (`[text](path)`), "embed" (`![[Note]]`, excluded)
- Two builders: buildGlobalGraph() (all notes) and buildLocalGraph(target, depth) (BFS neighborhood)
- Theme-aware coloring derived from palette

## 9. Interaction Model

- Pan: left-drag on empty space
- Zoom: mouse wheel (1.15x factor)
- Node hover: highlight, `nodeHovered(id)` signal
- Node click: select, `nodeClicked(id)`
- Node double-click: `nodeDoubleClicked(id)` (opens file)
- Right-click: context menu
- Pin node: fixes position during simulation

## 10. Performance Benchmarks

| Topology | Nodes | Edges | Time (ms) |
|----------|-------|-------|-----------|
| scale-free | 5000 | 9996 | 4643 |
| scale-free | 10000 | 19997 | 20342 |
| grid | 10000 | 19800 | 8545 |
| star | 10000 | 9999 | 15671 |

**Known limitations:**
- Main thread blocking during initial layout (async planned)
- Dense-clique topology bottlenecked by edge count

## 11. File Manifest

### Core Library (libs/forcegraph/)
| File | Lines | Purpose |
|------|-------|---------|
| `include/ForceGraphView.h` | 65 | View widget |
| `include/ForceGraphScene.h` | 65 | Scene management |
| `include/ForceGraphNode.h` | 31 | Node item |
| `include/ForceGraphEdge.h` | 28 | Edge item |
| `include/ForceLayoutEngine.h` | 71 | Physics simulation |
| `include/MultilevelLayout.h` | 45 | Walshaw coarsening |
| `include/QuadTree.h` | 36 | Barnes-Hut |
| `include/SpatialGrid.h` | 97 | Viewport culling (header-only) |
| `include/BatchNodeItem.h` | 45 | Batch node rendering |
| `include/BatchEdgeItem.h` | 33 | Batch edge rendering |
| `include/GraphTypes.h` | 36 | Type definitions |
| Tests: `tst_forcelayout.cpp`, `tst_quadtree.cpp`, `tst_benchmark_layout.cpp` | | |

### Application Layer (src/graph/)
| File | Lines | Purpose |
|------|-------|---------|
| `GraphViewTab.h/cpp` | 105 | Main graph tab |
| `GraphDataBuilder.h/cpp` | 25/240 | Data builder |
| `LocalGraphPanel.h/cpp` | 40 | Sidebar local graph |
| `GraphControlsPanel.h/cpp` | 107 | UI controls |

## 12. Reusability for Graffodil

**Directly transplantable:**
- ForceLayoutEngine → Graffodil::Force (ForceLayout)
- MultilevelLayout → included in Graffodil::Force
- QuadTree → Graffodil::Batch
- SpatialGrid → Graffodil::Batch (header-only)
- BatchNodeItem/BatchEdgeItem → Graffodil::Batch (BatchRenderer)

**Stays in Corbomite:**
- GraphDataBuilder (SQLiteIndex/VaultModel specific)
- GraphViewTab, LocalGraphPanel, GraphControlsPanel (app integration)
- ForceGraphNode/ForceGraphEdge (will implement Graffodil IGraphNode/IGraphEdge)
