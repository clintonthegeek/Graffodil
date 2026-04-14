#include <graffodil/DefaultGraphTool.h>
#include <graffodil/SelectMoveTool.h>
#include <graffodil/PanZoomTool.h>

namespace Graffodil {

DefaultGraphTool::DefaultGraphTool(QObject *parent)
    : CompositeTool(parent)
    , m_select(new SelectMoveTool(this))
    , m_panZoom(new PanZoomTool(this))
{
    // PanZoom is used purely for middle-button pan + Ctrl+wheel zoom.
    m_panZoom->setPanButton(Qt::MiddleButton);
    m_panZoom->setZoomButton(Qt::NoButton);  // no drag-to-zoom
    m_panZoom->setZoomWheelModifier(Qt::ControlModifier);

    // Mouse routes — first match wins.
    // Left button (no modifier, plus common extensions): select/move/rubber-band.
    // We register three matchers for the SelectMoveTool so Shift and Ctrl
    // modifier clicks also reach it.
    addMouseRoute(m_select, matchButton(Qt::LeftButton, Qt::NoModifier));
    addMouseRoute(m_select, matchButton(Qt::LeftButton, Qt::ShiftModifier));
    addMouseRoute(m_select, matchButton(Qt::LeftButton, Qt::ControlModifier));
    addMouseRoute(m_select, matchButton(Qt::LeftButton, Qt::MetaModifier));

    // Middle button: pan.
    addMouseRoute(m_panZoom, matchButton(Qt::MiddleButton));

    // Wheel: Ctrl+wheel → zoom. (Any wheel route forwards to panzoom,
    // which itself checks the modifier and returns early if it doesn't match.)
    addWheelRoute(m_panZoom, matchAnyWheel());

    // Keys go to SelectMoveTool (Delete, R).
    addKeyRoute(m_select);
}

DefaultGraphTool::~DefaultGraphTool() = default;

} // namespace Graffodil
