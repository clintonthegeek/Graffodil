#pragma once

#include <graffodil/CompositeTool.h>

namespace Graffodil {

class SelectMoveTool;
class PanZoomTool;

/// Prebuilt CompositeTool with the conventional desktop-app bindings:
/// - Left button: select / move / rubber-band (via an owned SelectMoveTool)
/// - Middle button drag: pan (via an owned PanZoomTool)
/// - Ctrl+wheel: zoom to cursor
/// - Delete/Backspace/R: delete / reverse selected (from SelectMoveTool)
///
/// Owns its sub-tools — destruction cleans them up. Access them via
/// selectMoveTool() / panZoomTool() to connect to their signals
/// (deleteRequested, nodeMoved, reverseRequested, etc.).
class DefaultGraphTool : public CompositeTool {
    Q_OBJECT
public:
    explicit DefaultGraphTool(QObject *parent = nullptr);
    ~DefaultGraphTool() override;

    SelectMoveTool *selectMoveTool() const { return m_select; }
    PanZoomTool *panZoomTool() const { return m_panZoom; }

private:
    SelectMoveTool *m_select;
    PanZoomTool *m_panZoom;
};

} // namespace Graffodil
