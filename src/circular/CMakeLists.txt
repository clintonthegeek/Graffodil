add_library(graffodil-circular SHARED)
add_library(Graffodil::Circular ALIAS graffodil-circular)
set_target_properties(graffodil-circular PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)
target_include_directories(graffodil-circular
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(graffodil-circular PUBLIC Graffodil::Core)

target_sources(graffodil-circular PRIVATE
    src/CircularLayout.cpp
    include/graffodil/CircularLayout.h
)

if(DEFINED KDE_INSTALL_LIBDIR)
    install(TARGETS graffodil-circular
        RUNTIME DESTINATION ${KDE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${KDE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${KDE_INSTALL_LIBDIR}
    )
endif()
