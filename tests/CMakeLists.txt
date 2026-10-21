# tests/CMakeLists.txt
# Test targets added incrementally as test files are created.

add_executable(tst_graphscene tst_graphscene.cpp)
target_link_libraries(tst_graphscene PRIVATE Graffodil::Core Qt6::Test)
target_include_directories(tst_graphscene PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_graphscene COMMAND tst_graphscene)

add_executable(tst_groups tst_groups.cpp)
target_link_libraries(tst_groups PRIVATE Graffodil::Core Qt6::Test)
target_include_directories(tst_groups PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_groups COMMAND tst_groups)

add_executable(tst_edgepathstrategies tst_edgepathstrategies.cpp)
target_link_libraries(tst_edgepathstrategies PRIVATE Graffodil::Core Qt6::Test)
add_test(NAME tst_edgepathstrategies COMMAND tst_edgepathstrategies)

add_executable(tst_terminus tst_terminus.cpp)
target_link_libraries(tst_terminus PRIVATE Graffodil::Core Qt6::Test)
add_test(NAME tst_terminus COMMAND tst_terminus)

add_executable(tst_graphedgeitem tst_graphedgeitem.cpp)
target_link_libraries(tst_graphedgeitem PRIVATE Graffodil::Core Qt6::Test)
target_include_directories(tst_graphedgeitem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_graphedgeitem COMMAND tst_graphedgeitem)

add_executable(tst_tools tst_tools.cpp)
target_link_libraries(tst_tools PRIVATE Graffodil::Core Qt6::Test)
target_include_directories(tst_tools PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_tools COMMAND tst_tools)

add_executable(tst_circular tst_circular.cpp)
target_link_libraries(tst_circular PRIVATE Graffodil::Circular Qt6::Test)
target_include_directories(tst_circular PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_circular COMMAND tst_circular)

add_executable(tst_sugiyama tst_sugiyama.cpp)
target_link_libraries(tst_sugiyama PRIVATE Graffodil::Sugiyama Qt6::Test)
target_include_directories(tst_sugiyama PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_sugiyama COMMAND tst_sugiyama)

add_executable(tst_spatialgrid tst_spatialgrid.cpp)
target_link_libraries(tst_spatialgrid PRIVATE Graffodil::Spatial Qt6::Test)
add_test(NAME tst_spatialgrid COMMAND tst_spatialgrid)

add_executable(tst_quadtree tst_quadtree.cpp)
target_link_libraries(tst_quadtree PRIVATE Graffodil::Spatial Qt6::Test)
add_test(NAME tst_quadtree COMMAND tst_quadtree)

add_executable(tst_forcelayout tst_forcelayout.cpp)
target_link_libraries(tst_forcelayout PRIVATE Graffodil::Force Qt6::Test)
target_include_directories(tst_forcelayout PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_forcelayout COMMAND tst_forcelayout)

add_executable(tst_batchrenderer tst_batchrenderer.cpp)
target_link_libraries(tst_batchrenderer PRIVATE Graffodil::Batch Qt6::Test)
# PROJECT_SOURCE_DIR, not CMAKE_SOURCE_DIR: this file is also consumed as a
# subdirectory (Corbomite adds libs/graffodil), and CMAKE_SOURCE_DIR is then
# the *host's* root, not Graffodil's. graffodil-batch's own PUBLIC include
# dir is only `include/`, so this one has to be named explicitly.
target_include_directories(tst_batchrenderer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/batch/src)
add_test(NAME tst_batchrenderer COMMAND tst_batchrenderer)

add_executable(tst_integration tst_integration.cpp)
target_link_libraries(tst_integration PRIVATE
    Graffodil::Core Graffodil::Circular Qt6::Test)
target_include_directories(tst_integration PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_integration COMMAND tst_integration)

add_executable(tst_minimap tst_minimap.cpp)
target_link_libraries(tst_minimap PRIVATE Graffodil::Core Qt6::Test)
add_test(NAME tst_minimap COMMAND tst_minimap)

add_executable(tst_edgelabels tst_edgelabels.cpp)
target_link_libraries(tst_edgelabels PRIVATE Graffodil::Core Qt6::Test)
target_include_directories(tst_edgelabels PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_edgelabels COMMAND tst_edgelabels)

add_executable(tst_dashoffset tst_dashoffset.cpp)
target_link_libraries(tst_dashoffset PRIVATE Graffodil::Core Qt6::Test)
target_include_directories(tst_dashoffset PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_dashoffset COMMAND tst_dashoffset)

# Every test above uses QTEST_MAIN, and these link Qt6::Widgets, so each one
# constructs a QApplication and needs a GUI platform. Force the offscreen
# platform so the suite runs on a headless box: with the default xcb plugin
# and no DISPLAY, QApplication aborts the process (SIGABRT) before the first
# test case runs. Naming the platform here also makes the suite deterministic
# on a machine that *does* have a display, where the same tests would
# otherwise open real windows.
set(GRAFFODIL_TEST_NAMES
    tst_graphscene
    tst_groups
    tst_edgepathstrategies
    tst_terminus
    tst_graphedgeitem
    tst_tools
    tst_circular
    tst_sugiyama
    tst_spatialgrid
    tst_quadtree
    tst_forcelayout
    tst_batchrenderer
    tst_integration
    tst_minimap
    tst_edgelabels
    tst_dashoffset)
set_tests_properties(${GRAFFODIL_TEST_NAMES} PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
