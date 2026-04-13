add_library(graffodil-sugiyama STATIC)
add_library(Graffodil::Sugiyama ALIAS graffodil-sugiyama)
set_target_properties(graffodil-sugiyama PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)
target_include_directories(graffodil-sugiyama
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(graffodil-sugiyama PUBLIC Graffodil::Core)

target_sources(graffodil-sugiyama PRIVATE
    src/SugiyamaLayout.cpp
    include/graffodil/SugiyamaLayout.h
)
