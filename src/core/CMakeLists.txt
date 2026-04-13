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

target_sources(graffodil-core PRIVATE src/placeholder.cpp)

# Sources added incrementally in later tasks
