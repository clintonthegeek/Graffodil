add_library(graffodil-batch STATIC)
add_library(Graffodil::Batch ALIAS graffodil-batch)
set_target_properties(graffodil-batch PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)
target_include_directories(graffodil-batch
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(graffodil-batch PUBLIC Graffodil::Core)
target_sources(graffodil-batch PRIVATE
    src/QuadTree.cpp
    src/BatchRenderer.cpp
    src/BatchNodeItem.cpp
    src/BatchEdgeItem.cpp
    include/graffodil/QuadTree.h
    include/graffodil/SpatialGrid.h
    include/graffodil/BatchRenderer.h
    src/BatchNodeItem.h
    src/BatchEdgeItem.h
)
