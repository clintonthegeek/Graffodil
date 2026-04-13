add_library(graffodil-circular STATIC)
add_library(Graffodil::Circular ALIAS graffodil-circular)
set_target_properties(graffodil-circular PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)
target_include_directories(graffodil-circular
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(graffodil-circular PUBLIC Graffodil::Core)
target_sources(graffodil-circular PRIVATE src/placeholder.cpp)
