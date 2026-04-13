#include <graffodil/GraphTool.h>
#include <graffodil/GraphScene.h>

namespace Graffodil {

void GraphTool::activate(GraphScene *scene)
{
    m_scene = scene;
}

void GraphTool::deactivate()
{
    m_scene = nullptr;
}

void GraphTool::keyPressEvent(QKeyEvent *) {}
void GraphTool::keyReleaseEvent(QKeyEvent *) {}

GraphScene *GraphTool::scene() const
{
    return m_scene;
}

} // namespace Graffodil
