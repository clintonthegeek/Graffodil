# src/core/CMakeLists.txt
add_library(graffodil-core STATIC)
add_library(Graffodil::Core ALIAS graffodil-core)

set_target_properties(graffodil-core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    AUTOMOC ON
)

target_include_directories(graffodil-core
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(graffodil-core PUBLIC Qt6::Core Qt6::Widgets)

target_sources(graffodil-core PRIVATE
    src/GraphScene.cpp
    src/EdgePathStrategy.cpp
    src/TerminusStyle.cpp
    src/GraphEdgeItem.cpp
    src/GraphTool.cpp
    src/SelectMoveTool.cpp
    src/CreateEdgeTool.cpp
    src/PanZoomTool.cpp
    src/AnchorHighlight.cpp
    src/GraphMinimap.cpp
    src/ConnectorItem.cpp
    include/graffodil/Types.h
    include/graffodil/IGraphNode.h
    include/graffodil/IGraphEdge.h
    include/graffodil/LayoutEngine.h
    include/graffodil/GraphScene.h
    include/graffodil/EdgePathStrategy.h
    include/graffodil/TerminusStyle.h
    include/graffodil/GraphEdgeItem.h
    include/graffodil/GraphTool.h
    include/graffodil/SelectMoveTool.h
    include/graffodil/CreateEdgeTool.h
    include/graffodil/PanZoomTool.h
    include/graffodil/AnchorHighlight.h
    include/graffodil/GraphMinimap.h
    include/graffodil/ConnectorItem.h
)
