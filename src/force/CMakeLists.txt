add_library(graffodil-force STATIC)
add_library(Graffodil::Force ALIAS graffodil-force)
set_target_properties(graffodil-force PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)
target_include_directories(graffodil-force
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(graffodil-force PUBLIC Graffodil::Core)
target_sources(graffodil-force PRIVATE src/placeholder.cpp)
