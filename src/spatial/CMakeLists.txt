add_library(graffodil-spatial STATIC)
add_library(Graffodil::Spatial ALIAS graffodil-spatial)
set_target_properties(graffodil-spatial PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(graffodil-spatial
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(graffodil-spatial PUBLIC Qt6::Core)
target_sources(graffodil-spatial PRIVATE
    src/QuadTree.cpp
    include/graffodil/QuadTree.h
    include/graffodil/SpatialGrid.h
)
