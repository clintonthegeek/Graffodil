#include "DemoWindow.h"

#include <graffodil/GraphScene.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include <graffodil/GraphEdgeItem.h>
#include <graffodil/EdgePathStrategy.h>
#include <graffodil/TerminusStyle.h>
#include <graffodil/SelectMoveTool.h>
#include <graffodil/CreateEdgeTool.h>
#include <graffodil/PanZoomTool.h>
#include <graffodil/GraphMinimap.h>
#include <graffodil/BatchRenderer.h>
#include <graffodil/CircularLayout.h>
#include <graffodil/SugiyamaLayout.h>
#include <graffodil/ForceLayout.h>

#include <QGraphicsView>
#include <QDockWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QRandomGenerator>
#include <QGraphicsRectItem>
#include <QStatusBar>

using namespace Graffodil;

// ---------------------------------------------------------------------------
// DemoNode — a visible rectangle node for the demo
// ---------------------------------------------------------------------------

class DemoNode : public QGraphicsRectItem, public IGraphNode {
public:
    DemoNode(const QString &id, const QPointF &pos = {})
        : QGraphicsRectItem(0, 0, 100, 60)
        , m_id(id)
    {
        setPos(pos);
        setFlag(ItemIsSelectable);
        setFlag(ItemIsMovable);
        setBrush(QColor(70, 130, 180));
        setPen(QPen(QColor(40, 80, 120), 1.5));

        // Label
        auto *label = new QGraphicsSimpleTextItem(id, this);
        label->setBrush(Qt::white);
        QFont f = label->font();
        f.setPointSize(9);
        f.setBold(true);
        label->setFont(f);
        // Center the label
        QRectF lr = label->boundingRect();
        label->setPos((100 - lr.width()) / 2, (60 - lr.height()) / 2);
    }

    QString nodeId() const override { return m_id; }

    QPointF anchorPoint(Side side) const override {
        const QRectF r = mapToScene(rect()).boundingRect();
        switch (side) {
        case Side::Top:    return {r.center().x(), r.top()};
        case Side::Bottom: return {r.center().x(), r.bottom()};
        case Side::Left:   return {r.left(), r.center().y()};
        case Side::Right:  return {r.right(), r.center().y()};
        }
        Q_UNREACHABLE();
    }

    QRectF nodeBoundingRect() const override {
        return mapToScene(rect()).boundingRect();
    }

    QGraphicsItem *graphicsItem() override { return this; }

private:
    QString m_id;
};

// ---------------------------------------------------------------------------
// DemoWindow
// ---------------------------------------------------------------------------

DemoWindow::DemoWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Graffodil Demo");

    // Scene + view
    m_scene = new GraphScene(this);
    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setDragMode(QGraphicsView::NoDrag);
    m_view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setCentralWidget(m_view);

    // Tools
    m_selectTool = new SelectMoveTool(this);
    m_createEdgeTool = new CreateEdgeTool(this);
    m_panZoomTool = new PanZoomTool(this);

    auto showStatus = [this](const QString &msg) { statusBar()->showMessage(msg); };
    connect(m_selectTool, &GraphTool::statusMessage, this, showStatus);
    connect(m_createEdgeTool, &GraphTool::statusMessage, this, showStatus);
    connect(m_panZoomTool, &GraphTool::statusMessage, this, showStatus);

    // Wire edge creation: when the tool emits edgeRequested, create a GraphEdgeItem
    connect(m_createEdgeTool, &CreateEdgeTool::edgeRequested,
            this, [this](IGraphNode *source, Side sourceSide,
                         IGraphNode *target, Side targetSide) {
        auto *edge = new GraphEdgeItem(source, sourceSide, target, targetSide,
                                        new BezierPathStrategy);
        edge->setTerminus(ArrowEnd::Target, std::make_unique<TriangleTerminus>());
        m_scene->addEdge(edge);
        edge->adjust();
        onEdgeCreated();
    });

    connect(m_selectTool, &SelectMoveTool::deleteRequested,
            this, &DemoWindow::onDeleteRequested);
    connect(m_selectTool, &SelectMoveTool::reverseRequested,
            this, &DemoWindow::onReverseRequested);

    m_scene->setActiveTool(m_selectTool);

    // Minimap
    m_minimap = new GraphMinimap(m_view);

    // Batch renderer
    m_batchRenderer = new BatchRenderer(m_scene);

    // Force layout timer
    m_forceTimer = new QTimer(this);
    m_forceTimer->setInterval(30);
    connect(m_forceTimer, &QTimer::timeout, this, &DemoWindow::forceLayoutTick);

    // Side panel
    buildSidePanel();

    // Status bar
    statusBar()->showMessage("Ready — generate a graph to begin");

    // Generate an initial graph
    generateGraph();
}

DemoWindow::~DemoWindow()
{
    m_forceTimer->stop();
    delete m_forceLayout;
}

void DemoWindow::buildSidePanel()
{
    auto *dock = new QDockWidget("Controls", this);
    dock->setFeatures(QDockWidget::DockWidgetMovable);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setMinimumWidth(240);

    auto *container = new QWidget;
    auto *layout = new QVBoxLayout(container);
    layout->setSpacing(8);

    // --- Graph Generation ---
    auto *genGroup = new QGroupBox("Graph Generation");
    auto *genLayout = new QVBoxLayout(genGroup);

    auto *nodeRow = new QHBoxLayout;
    nodeRow->addWidget(new QLabel("Nodes:"));
    m_nodeCountSpin = new QSpinBox;
    m_nodeCountSpin->setRange(3, 500);
    m_nodeCountSpin->setValue(15);
    nodeRow->addWidget(m_nodeCountSpin);
    genLayout->addLayout(nodeRow);

    auto *densityRow = new QHBoxLayout;
    densityRow->addWidget(new QLabel("Density:"));
    m_densitySpin = new QDoubleSpinBox;
    m_densitySpin->setRange(0.1, 1.0);
    m_densitySpin->setValue(0.3);
    m_densitySpin->setSingleStep(0.1);
    densityRow->addWidget(m_densitySpin);
    genLayout->addLayout(densityRow);

    auto *genBtn = new QPushButton("Generate");
    connect(genBtn, &QPushButton::clicked, this, &DemoWindow::generateGraph);
    genLayout->addWidget(genBtn);

    layout->addWidget(genGroup);

    // --- Layout ---
    auto *layoutGroup = new QGroupBox("Layout");
    auto *layoutLay = new QVBoxLayout(layoutGroup);

    m_layoutCombo = new QComboBox;
    m_layoutCombo->addItems({"Circular", "Sugiyama", "Force"});
    m_layoutCombo->setCurrentIndex(1); // default Sugiyama
    layoutLay->addWidget(m_layoutCombo);

    auto *layoutBtnRow = new QHBoxLayout;
    m_applyLayoutBtn = new QPushButton("Apply Layout");
    connect(m_applyLayoutBtn, &QPushButton::clicked, this, &DemoWindow::applyLayout);
    layoutBtnRow->addWidget(m_applyLayoutBtn);

    m_stopForceBtn = new QPushButton("Stop");
    m_stopForceBtn->setVisible(false);
    connect(m_stopForceBtn, &QPushButton::clicked, this, &DemoWindow::stopForceLayout);
    layoutBtnRow->addWidget(m_stopForceBtn);

    layoutLay->addLayout(layoutBtnRow);
    layout->addWidget(layoutGroup);

    // --- Tools ---
    auto *toolGroup = new QGroupBox("Tool");
    auto *toolLay = new QVBoxLayout(toolGroup);

    m_toolCombo = new QComboBox;
    m_toolCombo->addItems({"Select / Move", "Create Edge", "Pan / Zoom"});
    connect(m_toolCombo, &QComboBox::currentIndexChanged, this, &DemoWindow::onToolChanged);
    toolLay->addWidget(m_toolCombo);

    layout->addWidget(toolGroup);

    // --- Edge Appearance ---
    auto *edgeGroup = new QGroupBox("Edge Appearance");
    auto *edgeLay = new QVBoxLayout(edgeGroup);

    edgeLay->addWidget(new QLabel("Path Strategy:"));
    m_pathStrategyCombo = new QComboBox;
    m_pathStrategyCombo->addItems({"Direct", "Bezier", "Orthogonal", "Polyline"});
    m_pathStrategyCombo->setCurrentIndex(1); // Bezier default
    connect(m_pathStrategyCombo, &QComboBox::currentIndexChanged,
            this, &DemoWindow::onPathStrategyChanged);
    edgeLay->addWidget(m_pathStrategyCombo);

    edgeLay->addWidget(new QLabel("Source Terminus:"));
    m_sourceTerminusCombo = new QComboBox;
    m_sourceTerminusCombo->addItems({"None", "Triangle", "Open Arrow", "Diamond", "Circle", "Bar"});
    connect(m_sourceTerminusCombo, &QComboBox::currentIndexChanged,
            this, &DemoWindow::onSourceTerminusChanged);
    edgeLay->addWidget(m_sourceTerminusCombo);

    edgeLay->addWidget(new QLabel("Target Terminus:"));
    m_targetTerminusCombo = new QComboBox;
    m_targetTerminusCombo->addItems({"None", "Triangle", "Open Arrow", "Diamond", "Circle", "Bar"});
    m_targetTerminusCombo->setCurrentIndex(1); // Triangle default
    connect(m_targetTerminusCombo, &QComboBox::currentIndexChanged,
            this, &DemoWindow::onTargetTerminusChanged);
    edgeLay->addWidget(m_targetTerminusCombo);

    layout->addWidget(edgeGroup);

    // --- View ---
    auto *viewGroup = new QGroupBox("View");
    auto *viewLay = new QVBoxLayout(viewGroup);

    m_minimapCheck = new QCheckBox("Minimap");
    m_minimapCheck->setChecked(true);
    connect(m_minimapCheck, &QCheckBox::toggled, this, &DemoWindow::onMinimapToggled);
    viewLay->addWidget(m_minimapCheck);

    m_batchCheck = new QCheckBox("Batch Rendering");
    m_batchCheck->setChecked(false);
    connect(m_batchCheck, &QCheckBox::toggled, this, &DemoWindow::onBatchModeToggled);
    viewLay->addWidget(m_batchCheck);

    m_edgesAboveCheck = new QCheckBox("Edges above nodes");
    m_edgesAboveCheck->setChecked(false);
    connect(m_edgesAboveCheck, &QCheckBox::toggled, this, &DemoWindow::onEdgesAboveToggled);
    viewLay->addWidget(m_edgesAboveCheck);

    layout->addWidget(viewGroup);

    // --- Status ---
    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    layout->addStretch();

    scroll->setWidget(container);
    dock->setWidget(scroll);
    addDockWidget(Qt::RightDockWidgetArea, dock);
}

// ---------------------------------------------------------------------------
// Graph Generation
// ---------------------------------------------------------------------------

void DemoWindow::generateGraph()
{
    // Stop any running force simulation
    stopForceLayout();

    // Clear scene
    // Remove edges first, then nodes (edges reference nodes)
    for (auto *edge : m_scene->edges())
        m_scene->removeEdge(edge->edgeId());
    for (auto *node : m_scene->nodes())
        m_scene->removeNode(node->nodeId());

    int count = m_nodeCountSpin->value();
    double density = m_densitySpin->value();
    auto *rng = QRandomGenerator::global();

    // Create nodes
    QVector<DemoNode *> nodes;
    for (int i = 0; i < count; ++i) {
        auto *node = new DemoNode(QStringLiteral("N%1").arg(i));
        m_scene->addNode(node);
        nodes.append(node);
    }

    // Create edges: for each pair (i, j) where i < j, add with probability = density
    int edgeCount = 0;
    for (int i = 0; i < count; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (rng->generateDouble() < density) {
                auto *edge = new GraphEdgeItem(
                    nodes[i], Side::Right, nodes[j], Side::Left,
                    new BezierPathStrategy);
                edge->setTerminus(ArrowEnd::Target, std::make_unique<TriangleTerminus>());
                m_scene->addEdge(edge);
                ++edgeCount;
            }
        }
    }

    m_statusLabel->setText(QStringLiteral("%1 nodes, %2 edges").arg(count).arg(edgeCount));

    // Auto-apply current layout
    applyLayout();
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void DemoWindow::applyLayout()
{
    stopForceLayout();

    int layoutIdx = m_layoutCombo->currentIndex();
    auto sceneNodes = m_scene->nodes();
    auto sceneEdges = m_scene->edges();

    if (sceneNodes.isEmpty())
        return;

    if (layoutIdx == 0) {
        // Circular
        CircularLayout layout;
        layout.setRadius(qMax(50.0, sceneNodes.size() * 20.0));
        layout.setNodes(sceneNodes);
        layout.setEdges(sceneEdges);
        m_scene->applyLayout(layout.compute());
        adjustAllEdges();
        m_view->fitInView(m_scene->itemsBoundingRect().adjusted(-50, -50, 50, 50),
                          Qt::KeepAspectRatio);

    } else if (layoutIdx == 1) {
        // Sugiyama
        SugiyamaLayout layout;
        layout.setNodeSize({100, 60});
        layout.setSpacing(60, 40);
        layout.setNodes(sceneNodes);
        layout.setEdges(sceneEdges);
        m_scene->applyLayout(layout.compute());
        adjustAllEdges();
        m_view->fitInView(m_scene->itemsBoundingRect().adjusted(-50, -50, 50, 50),
                          Qt::KeepAspectRatio);

    } else if (layoutIdx == 2) {
        // Force — animated
        delete m_forceLayout;
        m_forceLayout = new ForceLayout;
        m_forceLayout->setNodes(sceneNodes);
        m_forceLayout->setEdges(sceneEdges);

        m_applyLayoutBtn->setEnabled(false);
        m_stopForceBtn->setVisible(true);
        m_forceTimer->start();
        statusBar()->showMessage("Force layout running...");
    }
}

void DemoWindow::stopForceLayout()
{
    m_forceTimer->stop();
    m_stopForceBtn->setVisible(false);
    m_applyLayoutBtn->setEnabled(true);
    if (m_forceLayout) {
        statusBar()->showMessage("Force layout stopped");
    }
}

void DemoWindow::forceLayoutTick()
{
    if (!m_forceLayout)
        return;

    bool running = m_forceLayout->tick();
    m_scene->applyLayout(m_forceLayout->currentPositions());
    adjustAllEdges();

    if (!running) {
        stopForceLayout();
        m_view->fitInView(m_scene->itemsBoundingRect().adjusted(-50, -50, 50, 50),
                          Qt::KeepAspectRatio);
        statusBar()->showMessage("Force layout converged");
    }
}

// ---------------------------------------------------------------------------
// Tools
// ---------------------------------------------------------------------------

void DemoWindow::onToolChanged(int index)
{
    switch (index) {
    case 0: m_scene->setActiveTool(m_selectTool); break;
    case 1: m_scene->setActiveTool(m_createEdgeTool); break;
    case 2: m_scene->setActiveTool(m_panZoomTool); break;
    }
}

void DemoWindow::onEdgeCreated()
{
    // Update new edge to match current appearance settings
    updateEdgeAppearance();
}

// ---------------------------------------------------------------------------
// Edge Appearance
// ---------------------------------------------------------------------------

static EdgePathStrategy *createStrategy(int index)
{
    switch (index) {
    case 0: return new DirectPathStrategy;
    case 1: return new BezierPathStrategy;
    case 2: return new OrthogonalPathStrategy;
    case 3: return new PolylinePathStrategy;
    default: return new BezierPathStrategy;
    }
}

static std::unique_ptr<TerminusStyle> createTerminus(int index)
{
    switch (index) {
    case 0: return std::make_unique<NoTerminus>();
    case 1: return std::make_unique<TriangleTerminus>();
    case 2: return std::make_unique<OpenArrowTerminus>();
    case 3: return std::make_unique<DiamondTerminus>();
    case 4: return std::make_unique<CircleTerminus>();
    case 5: return std::make_unique<BarTerminus>();
    default: return std::make_unique<NoTerminus>();
    }
}

void DemoWindow::updateEdgeAppearance()
{
    int stratIdx = m_pathStrategyCombo->currentIndex();
    int srcTermIdx = m_sourceTerminusCombo->currentIndex();
    int tgtTermIdx = m_targetTerminusCombo->currentIndex();

    for (auto *edge : m_scene->edges()) {
        auto *item = dynamic_cast<GraphEdgeItem *>(edge);
        if (!item) continue;

        item->setPathStrategy(createStrategy(stratIdx));
        item->setTerminus(ArrowEnd::Source, createTerminus(srcTermIdx));
        item->setTerminus(ArrowEnd::Target, createTerminus(tgtTermIdx));
        item->adjust();
    }
}

void DemoWindow::onPathStrategyChanged(int /*index*/)
{
    updateEdgeAppearance();
}

void DemoWindow::onSourceTerminusChanged(int /*index*/)
{
    updateEdgeAppearance();
}

void DemoWindow::onTargetTerminusChanged(int /*index*/)
{
    updateEdgeAppearance();
}

// ---------------------------------------------------------------------------
// View
// ---------------------------------------------------------------------------

void DemoWindow::onMinimapToggled(bool checked)
{
    m_minimap->setEnabled(checked);
}

void DemoWindow::onBatchModeToggled(bool checked)
{
    m_batchRenderer->setBatchMode(checked);
}

void DemoWindow::onEdgesAboveToggled(bool checked)
{
    m_scene->setEdgesAboveNodes(checked);
}

void DemoWindow::onDeleteRequested(const QList<IGraphNode *> &nodes,
                                   const QList<IGraphEdge *> &edges)
{
    // Delete edges first (they reference nodes)
    for (auto *edge : edges)
        m_scene->removeEdge(edge->edgeId());
    // For each node, also remove connected edges that weren't in the selection
    for (auto *node : nodes) {
        QString id = node->nodeId();
        // Collect edges to remove (modifying during iteration is unsafe)
        QList<QString> edgeIdsToRemove;
        for (auto *edge : m_scene->edges()) {
            if (edge->sourceNode()->nodeId() == id
                || edge->targetNode()->nodeId() == id) {
                edgeIdsToRemove.append(edge->edgeId());
            }
        }
        for (const auto &eid : edgeIdsToRemove)
            m_scene->removeEdge(eid);
        m_scene->removeNode(id);
    }

    // Delete the underlying QGraphicsItems (scene only holds non-owning refs
    // via interfaces, so we need to free the memory ourselves)
    for (auto *edge : edges)
        delete edge->graphicsItem();
    for (auto *node : nodes)
        delete node->graphicsItem();

    statusBar()->showMessage(
        QStringLiteral("Deleted %1 node(s), %2 edge(s)")
            .arg(nodes.size()).arg(edges.size()));
}

void DemoWindow::onReverseRequested(const QList<IGraphEdge *> &edges)
{
    int n = 0;
    for (auto *edge : edges) {
        if (auto *item = dynamic_cast<GraphEdgeItem *>(edge)) {
            item->reverse();
            ++n;
        }
    }
    statusBar()->showMessage(QStringLiteral("Reversed %1 edge(s)").arg(n));
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void DemoWindow::adjustAllEdges()
{
    for (auto *edge : m_scene->edges()) {
        auto *item = dynamic_cast<GraphEdgeItem *>(edge);
        if (item)
            item->adjust();
    }
}
