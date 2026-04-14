add_executable(graffodil-demo
    main.cpp
    DemoWindow.h
    DemoWindow.cpp
)

target_link_libraries(graffodil-demo PRIVATE
    Graffodil::Core
    Graffodil::Circular
    Graffodil::Sugiyama
    Graffodil::Force
    Graffodil::Batch
    Qt6::Widgets
)
