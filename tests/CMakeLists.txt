# tests/CMakeLists.txt
# Test targets added incrementally as test files are created.

add_executable(tst_graphscene tst_graphscene.cpp)
target_link_libraries(tst_graphscene PRIVATE Graffodil::Core Qt6::Test)
target_include_directories(tst_graphscene PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_graphscene COMMAND tst_graphscene)

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
target_link_libraries(tst_spatialgrid PRIVATE Graffodil::Batch Qt6::Test)
add_test(NAME tst_spatialgrid COMMAND tst_spatialgrid)

add_executable(tst_quadtree tst_quadtree.cpp)
target_link_libraries(tst_quadtree PRIVATE Graffodil::Batch Qt6::Test)
add_test(NAME tst_quadtree COMMAND tst_quadtree)
