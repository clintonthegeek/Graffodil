# src/core/CMakeLists.txt
# SHARED (Corbomite Cluster P Phase P6): a STATIC library gets a private
# copy of its Qt metaobjects/RTTI in every consumer, so a host and a
# dlopen()'d plugin that both touch a Graffodil type end up with two
# distinct QMetaObjects and qobject_cast silently fails across that
# boundary. No -fvisibility=hidden is set anywhere in this tree.
add_library(graffodil-core SHARED)
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
    src/IGraphNode.cpp
    src/Anchors.cpp
    src/Logging.cpp
    src/EdgePathStrategy.cpp
    src/TerminusStyle.cpp
    src/GraphEdgeItem.cpp
    src/GraphTool.cpp
    src/SelectMoveTool.cpp
    src/CreateEdgeTool.cpp
    src/PanZoomTool.cpp
    src/CompositeTool.cpp
    src/DefaultGraphTool.cpp
    src/AnchorHighlight.cpp
    src/GraphMinimap.cpp
    src/ConnectorItem.cpp
    src/GroupItem.cpp
    include/graffodil/Types.h
    include/graffodil/IGraphNode.h
    include/graffodil/IRenderableCache.h
    include/graffodil/IAlignmentStrategy.h
    include/graffodil/Anchors.h
    include/graffodil/Logging.h
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
    include/graffodil/CompositeTool.h
    include/graffodil/DefaultGraphTool.h
    include/graffodil/AnchorHighlight.h
    include/graffodil/GraphMinimap.h
    include/graffodil/ConnectorItem.h
    include/graffodil/GroupStyle.h
    include/graffodil/GroupItem.h
)

# Install rule — only meaningful when built as a Corbomite subdirectory
# (KDEInstallDirs is included by the parent tree before add_subdirectory);
# a standalone Graffodil build skips this, same as before this library
# was SHARED.
if(DEFINED KDE_INSTALL_LIBDIR)
    install(TARGETS graffodil-core
        RUNTIME DESTINATION ${KDE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${KDE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${KDE_INSTALL_LIBDIR}
    )
endif()
