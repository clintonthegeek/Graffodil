#include "DemoWindow.h"
#include "DemoNodes.h"

#include <graffodil/GraphScene.h>
#include <graffodil/IGraphNode.h>
#include <graffodil/IGraphEdge.h>
#include <graffodil/Anchors.h>
#include <graffodil/GraphEdgeItem.h>
#include <graffodil/EdgePathStrategy.h>
#include <graffodil/TerminusStyle.h>
#include <graffodil/SelectMoveTool.h>
#include <graffodil/CreateEdgeTool.h>
#include <graffodil/PanZoomTool.h>
#include <graffodil/CompositeTool.h>
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
#include <QFormLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QTimer>
#include <QRandomGenerator>
#include <QStatusBar>
#include <QSet>
#include <QShowEvent>

using namespace Graffodil;

// ---------------------------------------------------------------------------
// Template definitions
// ---------------------------------------------------------------------------

enum TemplateIndex {
    Tpl_RandomDAG = 0,
    Tpl_LinearChain,
    Tpl_BinaryTree,
    Tpl_SignalChain,
    Tpl_SwivelCluster,
    Tpl_Mixed,
    Tpl_Empty,
};

static const char *templateNames[] = {
    "Random DAG  (Compass nodes)",
    "Linear chain  (Dependency nodes)",
    "Binary tree  (Compass nodes)",
    "Signal chain  (Port nodes)",
    "Swivel cluster  (Swivel nodes)",
    "Mixed showcase  (one of each)",
    "Empty canvas",
};

// ---------------------------------------------------------------------------
// DemoWindow
// ---------------------------------------------------------------------------

DemoWindow::DemoWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Graffodil Demo");

    m_scene = new GraphScene(this);
    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setDragMode(QGraphicsView::NoDrag);
    m_view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    m_view->setMouseTracking(true);
    setCentralWidget(m_view);

    // Tools. CreateEdgeTool runs in drag-only mode since it's embedded in
    // the unified composite via anchor routing — release without drag
    // cancels rather than waiting for a second click.
    m_selectMoveTool = new SelectMoveTool(this);
    m_createEdgeTool = new CreateEdgeTool(this);
    m_createEdgeTool->setDragOnly(true);
    m_panZoomTool = new PanZoomTool(this);
    m_panZoomTool->setPanButton(Qt::MiddleButton);
    m_panZoomTool->setZoomButton(Qt::NoButton);
    m_panZoomTool->setZoomWheelModifier(Qt::NoModifier);

    auto showStatus = [this](const QString &msg) { statusBar()->showMessage(msg); };
    connect(m_selectMoveTool, &GraphTool::statusMessage, this, showStatus);
    connect(m_createEdgeTool, &GraphTool::statusMessage, this, showStatus);
    connect(m_panZoomTool, &GraphTool::statusMessage, this, showStatus);

    connect(m_createEdgeTool, &CreateEdgeTool::edgeRequested,
            this, [this](IGraphNode *source, const QString &sourceAnchorId,
                         IGraphNode *target, const QString &targetAnchorId) {
        auto *edge = new GraphEdgeItem(source, sourceAnchorId,
                                        target, targetAnchorId,
                                        new BezierPathStrategy);
        edge->setTerminus(ArrowEnd::Target, std::make_unique<TriangleTerminus>());
        m_scene->addEdge(edge);
        edge->adjust();
        onEdgeCreated();
    });

    connect(m_selectMoveTool, &SelectMoveTool::deleteRequested,
            this, &DemoWindow::onDeleteRequested);
    connect(m_selectMoveTool, &SelectMoveTool::reverseRequested,
            this, &DemoWindow::onReverseRequested);

    // Single unified tool. Routing priority (first match wins):
    //  1. Left-button press within 12px of any anchor → CreateEdgeTool
    //     (drag to another anchor to create; release without drag cancels).
    //  2. Left-button elsewhere → SelectMoveTool (select/move/rubber-band).
    //  3. Middle-button → PanZoomTool pan.
    //  4. Wheel → PanZoomTool zoom-to-cursor.
    //  5. Keys → SelectMoveTool (Delete/Backspace/R) and CreateEdgeTool (Esc).
    m_unifiedTool = new CompositeTool(this);
    m_unifiedTool->addAnchorRoute(m_createEdgeTool, 8.0);
    m_unifiedTool->addMouseRoute(m_selectMoveTool,
                                  CompositeTool::matchButton(Qt::LeftButton, Qt::NoModifier));
    m_unifiedTool->addMouseRoute(m_selectMoveTool,
                                  CompositeTool::matchButton(Qt::LeftButton, Qt::ShiftModifier));
    m_unifiedTool->addMouseRoute(m_selectMoveTool,
                                  CompositeTool::matchButton(Qt::LeftButton, Qt::ControlModifier));
    m_unifiedTool->addMouseRoute(m_panZoomTool,
                                  CompositeTool::matchButton(Qt::MiddleButton));
    m_unifiedTool->addWheelRoute(m_panZoomTool, CompositeTool::matchAnyWheel());
    m_unifiedTool->addKeyRoute(m_selectMoveTool);
    m_unifiedTool->addKeyRoute(m_createEdgeTool);

    m_scene->setActiveTool(m_unifiedTool);

    m_minimap = new GraphMinimap(m_view);
    m_batchRenderer = new BatchRenderer(m_scene);

    m_forceTimer = new QTimer(this);
    m_forceTimer->setInterval(30);
    connect(m_forceTimer, &QTimer::timeout, this, &DemoWindow::forceLayoutTick);

    buildSidePanel();

    statusBar()->showMessage("Ready — pick a template and click Generate");
    generateGraph();
}

DemoWindow::~DemoWindow()
{
    m_forceTimer->stop();
    delete m_forceLayout;
}

// ---------------------------------------------------------------------------
// Side panel
// ---------------------------------------------------------------------------

void DemoWindow::buildSidePanel()
{
    auto *dock = new QDockWidget("Controls", this);
    dock->setFeatures(QDockWidget::DockWidgetMovable);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setMinimumWidth(280);

    auto *container = new QWidget;
    auto *layout = new QVBoxLayout(container);
    layout->setSpacing(8);

    // --- Graph ---
    auto *graphGroup = new QGroupBox("Graph");
    auto *graphLay = new QVBoxLayout(graphGroup);

    graphLay->addWidget(new QLabel("Template:"));
    m_templateCombo = new QComboBox;
    for (const char *name : templateNames)
        m_templateCombo->addItem(name);
    connect(m_templateCombo, &QComboBox::currentIndexChanged,
            this, &DemoWindow::onTemplateChanged);
    graphLay->addWidget(m_templateCombo);

    // Per-template parameter pages — shown/hidden by onTemplateChanged.
    m_paramStack = new QStackedWidget;

    // Random DAG params
    {
        auto *page = new QWidget;
        auto *form = new QFormLayout(page);
        form->setContentsMargins(0, 0, 0, 0);
        m_nodeCountSpin = new QSpinBox;
        m_nodeCountSpin->setRange(3, 500);
        m_nodeCountSpin->setValue(15);
        form->addRow("Nodes:", m_nodeCountSpin);
        m_densitySpin = new QDoubleSpinBox;
        m_densitySpin->setRange(0.05, 1.0);
        m_densitySpin->setSingleStep(0.05);
        m_densitySpin->setValue(0.3);
        form->addRow("Density:", m_densitySpin);
        m_paramStack->addWidget(page);
    }
    // Linear chain params
    {
        auto *page = new QWidget;
        auto *form = new QFormLayout(page);
        form->setContentsMargins(0, 0, 0, 0);
        m_chainLengthSpin = new QSpinBox;
        m_chainLengthSpin->setRange(2, 30);
        m_chainLengthSpin->setValue(6);
        form->addRow("Length:", m_chainLengthSpin);
        m_paramStack->addWidget(page);
    }
    // Binary tree params
    {
        auto *page = new QWidget;
        auto *form = new QFormLayout(page);
        form->setContentsMargins(0, 0, 0, 0);
        m_treeDepthSpin = new QSpinBox;
        m_treeDepthSpin->setRange(1, 6);
        m_treeDepthSpin->setValue(4);
        form->addRow("Depth:", m_treeDepthSpin);
        m_paramStack->addWidget(page);
    }
    // Signal chain params
    {
        auto *page = new QWidget;
        auto *form = new QFormLayout(page);
        form->setContentsMargins(0, 0, 0, 0);
        m_signalLengthSpin = new QSpinBox;
        m_signalLengthSpin->setRange(2, 12);
        m_signalLengthSpin->setValue(4);
        form->addRow("Plugins:", m_signalLengthSpin);
        m_paramStack->addWidget(page);
    }
    // Swivel cluster params
    {
        auto *page = new QWidget;
        auto *form = new QFormLayout(page);
        form->setContentsMargins(0, 0, 0, 0);
        m_swivelCountSpin = new QSpinBox;
        m_swivelCountSpin->setRange(3, 30);
        m_swivelCountSpin->setValue(7);
        form->addRow("Nodes:", m_swivelCountSpin);
        m_paramStack->addWidget(page);
    }
    // Mixed / Empty: no params
    m_paramStack->addWidget(new QWidget);
    m_paramStack->addWidget(new QWidget);

    graphLay->addWidget(m_paramStack);

    auto *graphBtns = new QHBoxLayout;
    auto *genBtn = new QPushButton("Generate");
    connect(genBtn, &QPushButton::clicked, this, &DemoWindow::generateGraph);
    graphBtns->addWidget(genBtn);
    auto *clearBtn = new QPushButton("Clear");
    connect(clearBtn, &QPushButton::clicked, this, &DemoWindow::clearGraph);
    graphBtns->addWidget(clearBtn);
    graphLay->addLayout(graphBtns);

    layout->addWidget(graphGroup);

    // --- Layout ---
    auto *layoutGroup = new QGroupBox("Layout");
    auto *layoutLay = new QVBoxLayout(layoutGroup);
    m_layoutCombo = new QComboBox;
    m_layoutCombo->addItems({"Circular", "Sugiyama", "Force (animated)"});
    m_layoutCombo->setCurrentIndex(1);
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

    // --- Interaction help ---
    auto *helpGroup = new QGroupBox("Interaction");
    auto *helpLay = new QVBoxLayout(helpGroup);
    auto *helpText = new QLabel(
        "<b>Drag from any anchor</b> (visible dots on nodes) to another "
        "anchor to create an edge.<br>"
        "<b>Click</b> a node or edge to select. <b>Shift</b>-click to add, "
        "<b>Ctrl</b>-click to toggle.<br>"
        "<b>Drag</b> a node to move. Drag empty space to rubber-band.<br>"
        "<b>Middle-drag</b> to pan. <b>Wheel</b> to zoom.<br>"
        "<b>Del</b> to remove selected. <b>R</b> to reverse selected edges.");
    helpText->setWordWrap(true);
    helpText->setStyleSheet("font-size: 10pt; color: #666;");
    helpLay->addWidget(helpText);
    layout->addWidget(helpGroup);

    // --- Edge Appearance ---
    auto *edgeGroup = new QGroupBox("Edge Appearance");
    auto *edgeLay = new QVBoxLayout(edgeGroup);

    edgeLay->addWidget(new QLabel("Path Strategy:"));
    m_pathStrategyCombo = new QComboBox;
    m_pathStrategyCombo->addItems({"Direct", "Bezier", "Orthogonal", "Polyline"});
    m_pathStrategyCombo->setCurrentIndex(1);
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
    m_targetTerminusCombo->setCurrentIndex(1);
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
    connect(m_batchCheck, &QCheckBox::toggled, this, &DemoWindow::onBatchModeToggled);
    viewLay->addWidget(m_batchCheck);

    m_edgesAboveCheck = new QCheckBox("Edges above nodes");
    connect(m_edgesAboveCheck, &QCheckBox::toggled, this, &DemoWindow::onEdgesAboveToggled);
    viewLay->addWidget(m_edgesAboveCheck);

    auto *zoomBtnRow = new QHBoxLayout;
    auto *fitBtn = new QPushButton("Zoom to Fit");
    connect(fitBtn, &QPushButton::clicked, this, &DemoWindow::zoomToFit);
    zoomBtnRow->addWidget(fitBtn);
    auto *resetBtn = new QPushButton("1:1");
    resetBtn->setToolTip("Reset zoom to 1:1");
    connect(resetBtn, &QPushButton::clicked, this, &DemoWindow::resetZoom);
    zoomBtnRow->addWidget(resetBtn);
    viewLay->addLayout(zoomBtnRow);
    layout->addWidget(viewGroup);

    // --- Status ---
    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    layout->addStretch();
    scroll->setWidget(container);
    dock->setWidget(scroll);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    // Initialize template visibility.
    onTemplateChanged(m_templateCombo->currentIndex());
}

void DemoWindow::onTemplateChanged(int index)
{
    m_paramStack->setCurrentIndex(index);
}

// ---------------------------------------------------------------------------
// Generators
// ---------------------------------------------------------------------------

static GraphEdgeItem *makeEdge(IGraphNode *src, const QString &srcId,
                                IGraphNode *tgt, const QString &tgtId)
{
    auto *e = new GraphEdgeItem(src, srcId, tgt, tgtId, new BezierPathStrategy);
    e->setTerminus(ArrowEnd::Target, std::make_unique<TriangleTerminus>());
    return e;
}

static void genRandomDAG(GraphScene *scene, int count, double density)
{
    auto *rng = QRandomGenerator::global();
    QVector<CompassNode *> nodes;
    for (int i = 0; i < count; ++i) {
        auto *n = new CompassNode(QStringLiteral("N%1").arg(i));
        scene->addNode(n);
        nodes.append(n);
    }
    for (int i = 0; i < count; ++i)
        for (int j = i + 1; j < count; ++j)
            if (rng->generateDouble() < density)
                scene->addEdge(makeEdge(nodes[i], AnchorIds::Right,
                                         nodes[j], AnchorIds::Left));
}

static void genLinearChain(GraphScene *scene, int length)
{
    QVector<DependencyNode *> nodes;
    for (int i = 0; i < length; ++i) {
        auto *n = new DependencyNode(QStringLiteral("Step %1").arg(i + 1));
        scene->addNode(n);
        nodes.append(n);
    }
    for (int i = 0; i + 1 < length; ++i)
        scene->addEdge(makeEdge(nodes[i], AnchorIds::Right,
                                 nodes[i + 1], AnchorIds::Left));
}

static void genBinaryTree(GraphScene *scene, int depth)
{
    QVector<CompassNode *> nodes;
    const int total = (1 << depth) - 1;   // 2^depth - 1
    for (int i = 0; i < total; ++i) {
        auto *n = new CompassNode(QStringLiteral("N%1").arg(i));
        scene->addNode(n);
        nodes.append(n);
    }
    for (int i = 0; i < total; ++i) {
        const int left = 2 * i + 1;
        const int right = 2 * i + 2;
        if (left < total)
            scene->addEdge(makeEdge(nodes[i], AnchorIds::Bottom,
                                     nodes[left], AnchorIds::Top));
        if (right < total)
            scene->addEdge(makeEdge(nodes[i], AnchorIds::Bottom,
                                     nodes[right], AnchorIds::Top));
    }
}

static void genSignalChain(GraphScene *scene, int length)
{
    // A chain of audio plugins with stereo signal + occasional sidechain.
    QVector<PortNode *> plugins;
    for (int i = 0; i < length; ++i) {
        const QString id = QStringLiteral("Plugin %1").arg(i + 1);
        PortNode *n;
        if (i == 0)
            n = new PortNode(id, {}, {"L", "R"});
        else if (i == length - 1)
            n = new PortNode(id, {"L", "R"}, {});
        else
            n = new PortNode(id, {"L", "R", "side"}, {"L", "R"});
        scene->addNode(n);
        plugins.append(n);
    }
    // Stereo wiring along the chain.
    for (int i = 0; i + 1 < length; ++i) {
        scene->addEdge(makeEdge(plugins[i], "out:L", plugins[i + 1], "in:L"));
        scene->addEdge(makeEdge(plugins[i], "out:R", plugins[i + 1], "in:R"));
    }
    // Sidechain into a middle plugin. Middle plugins are the only ones
    // with an "in:side" anchor (the last plugin has inputs but no
    // sidechain). length-2 is always a middle plugin when length >= 3.
    if (length >= 3) {
        const int sideTarget = length - 2;
        scene->addEdge(makeEdge(plugins[0], "out:L",
                                 plugins[sideTarget], "in:side"));
    }
}

static void genSwivelCluster(GraphScene *scene, int count)
{
    // A ring of swivel nodes plus a central hub — demonstrates dynamic
    // anchors since edges attach wherever the boundaries face.
    QVector<SwivelNode *> nodes;
    for (int i = 0; i < count; ++i) {
        auto *n = new SwivelNode(QString::number(i + 1));
        scene->addNode(n);
        nodes.append(n);
    }
    // Connect each to its neighbor (ring) with empty anchor ids — swivel.
    for (int i = 0; i < count; ++i) {
        const int j = (i + 1) % count;
        scene->addEdge(makeEdge(nodes[i], QString(), nodes[j], QString()));
    }
    // Plus a hub connection from [0] to all others.
    for (int i = 2; i < count; ++i)
        scene->addEdge(makeEdge(nodes[0], QString(), nodes[i], QString()));
}

static void genMixed(GraphScene *scene)
{
    // One of each node type, wired together to show heterogeneous graphs.
    auto *compass = new CompassNode("Compass");
    auto *dep = new DependencyNode("Dependency");
    auto *plug = new PortNode("Plugin", {"L", "R"}, {"L", "R"});
    auto *swiv = new SwivelNode("Swivel");
    scene->addNode(compass);
    scene->addNode(dep);
    scene->addNode(plug);
    scene->addNode(swiv);

    scene->addEdge(makeEdge(compass, AnchorIds::Right, dep, AnchorIds::Left));
    scene->addEdge(makeEdge(dep, AnchorIds::Right, plug, "in:L"));
    scene->addEdge(makeEdge(plug, "out:R", swiv, QString()));
    scene->addEdge(makeEdge(swiv, QString(), compass, AnchorIds::Bottom));
}

void DemoWindow::generateGraph()
{
    stopForceLayout();

    // Clear everything first — free the graphics items the scene no longer owns.
    auto cleared = m_scene->clearGraph();
    for (auto *edge : cleared.edges)
        delete edge->graphicsItem();
    for (auto *node : cleared.nodes)
        delete node->graphicsItem();

    const int tpl = m_templateCombo->currentIndex();
    switch (tpl) {
    case Tpl_RandomDAG:
        genRandomDAG(m_scene, m_nodeCountSpin->value(), m_densitySpin->value());
        break;
    case Tpl_LinearChain:
        genLinearChain(m_scene, m_chainLengthSpin->value());
        break;
    case Tpl_BinaryTree:
        genBinaryTree(m_scene, m_treeDepthSpin->value());
        break;
    case Tpl_SignalChain:
        genSignalChain(m_scene, m_signalLengthSpin->value());
        break;
    case Tpl_SwivelCluster:
        genSwivelCluster(m_scene, m_swivelCountSpin->value());
        break;
    case Tpl_Mixed:
        genMixed(m_scene);
        break;
    case Tpl_Empty:
        // Leave blank for manual building.
        break;
    }

    // Apply current appearance settings to the new edges.
    updateEdgeAppearance();

    m_statusLabel->setText(QStringLiteral("%1 nodes, %2 edges")
                               .arg(m_scene->nodes().size())
                               .arg(m_scene->edges().size()));

    // Auto-layout (skip for empty/swivel — swivel nodes are circular and
    // Sugiyama assumes directional flow which doesn't match a ring).
    if (tpl != Tpl_Empty && tpl != Tpl_SwivelCluster) {
        applyLayout();
    } else if (tpl == Tpl_SwivelCluster) {
        // Arrange in a circle manually.
        CircularLayout layout;
        layout.setRadius(qMax(100.0, m_scene->nodes().size() * 30.0));
        layout.setNodes(m_scene->nodes());
        layout.setEdges(m_scene->edges());
        m_scene->applyLayout(layout.compute());
        fitViewToScene();
    } else {
        fitViewToScene();
    }
}

void DemoWindow::clearGraph()
{
    stopForceLayout();
    auto cleared = m_scene->clearGraph();
    for (auto *edge : cleared.edges)
        delete edge->graphicsItem();
    for (auto *node : cleared.nodes)
        delete node->graphicsItem();
    m_statusLabel->setText("0 nodes, 0 edges");
    statusBar()->showMessage("Cleared");
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void DemoWindow::applyLayout()
{
    stopForceLayout();

    auto sceneNodes = m_scene->nodes();
    auto sceneEdges = m_scene->edges();
    if (sceneNodes.isEmpty())
        return;

    const int idx = m_layoutCombo->currentIndex();
    if (idx == 0) {
        CircularLayout layout;
        layout.setRadius(qMax(50.0, sceneNodes.size() * 20.0));
        layout.setNodes(sceneNodes);
        layout.setEdges(sceneEdges);
        m_scene->applyLayout(layout.compute());
        fitViewToScene();
    } else if (idx == 1) {
        SugiyamaLayout layout;
        layout.setNodeSize({120, 80});
        layout.setSpacing(60, 40);
        layout.setNodes(sceneNodes);
        layout.setEdges(sceneEdges);
        m_scene->applyLayout(layout.compute());
        fitViewToScene();
    } else if (idx == 2) {
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
    if (m_forceLayout)
        statusBar()->showMessage("Force layout stopped");
}

void DemoWindow::forceLayoutTick()
{
    if (!m_forceLayout)
        return;
    const bool running = m_forceLayout->tick();
    m_scene->applyLayout(m_forceLayout->currentPositions());
    if (!running) {
        stopForceLayout();
        fitViewToScene();
        statusBar()->showMessage("Force layout converged");
    }
}

// ---------------------------------------------------------------------------
// Tools & edge appearance
// ---------------------------------------------------------------------------

void DemoWindow::onEdgeCreated()
{
    updateEdgeAppearance();
}

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
    const int stratIdx = m_pathStrategyCombo->currentIndex();
    const int srcTermIdx = m_sourceTerminusCombo->currentIndex();
    const int tgtTermIdx = m_targetTerminusCombo->currentIndex();

    for (auto *edge : m_scene->edges()) {
        auto *item = dynamic_cast<GraphEdgeItem *>(edge);
        if (!item) continue;
        item->setPathStrategy(createStrategy(stratIdx));
        item->setTerminus(ArrowEnd::Source, createTerminus(srcTermIdx));
        item->setTerminus(ArrowEnd::Target, createTerminus(tgtTermIdx));
        item->adjust();
    }
}

void DemoWindow::onPathStrategyChanged(int)  { updateEdgeAppearance(); }
void DemoWindow::onSourceTerminusChanged(int) { updateEdgeAppearance(); }
void DemoWindow::onTargetTerminusChanged(int) { updateEdgeAppearance(); }

// ---------------------------------------------------------------------------
// View
// ---------------------------------------------------------------------------

void DemoWindow::onMinimapToggled(bool checked) { m_minimap->setEnabled(checked); }
void DemoWindow::onBatchModeToggled(bool checked) { m_batchRenderer->setBatchMode(checked); }
void DemoWindow::onEdgesAboveToggled(bool checked) { m_scene->setEdgesAboveNodes(checked); }

void DemoWindow::fitViewToScene()
{
    m_scene->fitSceneRectToContent(1000.0);
    QRectF content = m_scene->itemsBoundingRect();
    if (content.isEmpty())
        return;
    m_view->fitInView(content.adjusted(-50, -50, 50, 50), Qt::KeepAspectRatio);
}

void DemoWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (!m_initialFitDone) {
        m_initialFitDone = true;
        fitViewToScene();
    }
}

void DemoWindow::zoomToFit()
{
    fitViewToScene();
    statusBar()->showMessage("Zoomed to fit");
}

void DemoWindow::resetZoom()
{
    m_view->resetTransform();
    QRectF content = m_scene->itemsBoundingRect();
    if (!content.isEmpty())
        m_view->centerOn(content.center());
    statusBar()->showMessage("Zoom reset to 1:1");
}

// ---------------------------------------------------------------------------
// Delete / reverse
// ---------------------------------------------------------------------------

void DemoWindow::onDeleteRequested(const QList<IGraphNode *> &nodes,
                                   const QList<IGraphEdge *> &edges)
{
    QSet<IGraphEdge *> toDeleteEdges(edges.begin(), edges.end());
    for (auto *node : nodes) {
        const auto connected = m_scene->connectedEdges(node->nodeId());
        for (auto *e : connected)
            toDeleteEdges.insert(e);
    }
    for (auto *edge : edges)
        m_scene->removeEdge(edge->edgeId());
    for (auto *node : nodes)
        m_scene->removeNode(node->nodeId());
    for (auto *edge : toDeleteEdges)
        delete edge->graphicsItem();
    for (auto *node : nodes)
        delete node->graphicsItem();

    m_statusLabel->setText(QStringLiteral("%1 nodes, %2 edges")
                               .arg(m_scene->nodes().size())
                               .arg(m_scene->edges().size()));
    statusBar()->showMessage(
        QStringLiteral("Deleted %1 node(s), %2 edge(s)")
            .arg(nodes.size()).arg(toDeleteEdges.size()));
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
