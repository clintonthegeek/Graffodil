add_library(graffodil-force SHARED)
add_library(Graffodil::Force ALIAS graffodil-force)
set_target_properties(graffodil-force PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)
target_include_directories(graffodil-force
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(graffodil-force PUBLIC Graffodil::Core PRIVATE Graffodil::Spatial)

target_sources(graffodil-force PRIVATE
    src/ForceLayout.cpp
    src/MultilevelLayout.cpp
    include/graffodil/ForceLayout.h
    include/graffodil/MultilevelLayout.h
)

if(DEFINED KDE_INSTALL_LIBDIR)
    install(TARGETS graffodil-force
        RUNTIME DESTINATION ${KDE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${KDE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${KDE_INSTALL_LIBDIR}
    )
endif()
