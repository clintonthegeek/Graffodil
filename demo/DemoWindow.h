#pragma once

#include <QMainWindow>

class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QPushButton;
class QTimer;
class QGraphicsView;
class QLabel;

namespace Graffodil {
class GraphScene;
class GraphMinimap;
class BatchRenderer;
class DefaultGraphTool;
class CreateEdgeTool;
class ForceLayout;
class IGraphNode;
class IGraphEdge;
}

class DemoWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit DemoWindow(QWidget *parent = nullptr);
    ~DemoWindow() override;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void generateGraph();
    void applyLayout();
    void stopForceLayout();
    void forceLayoutTick();
    void zoomToFit();
    void resetZoom();
    void onToolChanged(int index);
    void onPathStrategyChanged(int index);
    void onSourceTerminusChanged(int index);
    void onTargetTerminusChanged(int index);
    void onMinimapToggled(bool checked);
    void onBatchModeToggled(bool checked);
    void onEdgesAboveToggled(bool checked);
    void onEdgeCreated();
    void onDeleteRequested(const QList<Graffodil::IGraphNode *> &nodes,
                           const QList<Graffodil::IGraphEdge *> &edges);
    void onReverseRequested(const QList<Graffodil::IGraphEdge *> &edges);

private:
    void buildSidePanel();
    void updateEdgeAppearance();
    void adjustAllEdges();
    void fitViewToScene();

    bool m_initialFitDone = false;

    // Scene
    Graffodil::GraphScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;
    Graffodil::GraphMinimap *m_minimap = nullptr;
    Graffodil::BatchRenderer *m_batchRenderer = nullptr;

    // Tools — DefaultGraphTool is a composite handling left=select/move,
    // middle=pan, Ctrl+wheel=zoom. CreateEdgeTool remains a separate modal
    // tool the user switches into when adding edges.
    Graffodil::DefaultGraphTool *m_defaultTool = nullptr;
    Graffodil::CreateEdgeTool *m_createEdgeTool = nullptr;

    // Force layout animation
    Graffodil::ForceLayout *m_forceLayout = nullptr;
    QTimer *m_forceTimer = nullptr;

    // Controls
    QSpinBox *m_nodeCountSpin = nullptr;
    QDoubleSpinBox *m_densitySpin = nullptr;
    QComboBox *m_layoutCombo = nullptr;
    QPushButton *m_applyLayoutBtn = nullptr;
    QPushButton *m_stopForceBtn = nullptr;
    QComboBox *m_toolCombo = nullptr;
    QComboBox *m_pathStrategyCombo = nullptr;
    QComboBox *m_sourceTerminusCombo = nullptr;
    QComboBox *m_targetTerminusCombo = nullptr;
    QCheckBox *m_minimapCheck = nullptr;
    QCheckBox *m_batchCheck = nullptr;
    QCheckBox *m_edgesAboveCheck = nullptr;
    QLabel *m_statusLabel = nullptr;
};
