# tests/CMakeLists.txt
# Test targets added incrementally as test files are created.

add_executable(tst_graphscene tst_graphscene.cpp)
target_link_libraries(tst_graphscene PRIVATE Graffodil::Core Qt6::Test)
target_include_directories(tst_graphscene PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tst_graphscene COMMAND tst_graphscene)
