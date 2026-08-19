cmake_minimum_required(VERSION 3.19)
project(Graffodil VERSION 0.2.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Widgets Test)

add_subdirectory(src/core)
add_subdirectory(src/spatial)
add_subdirectory(src/sugiyama)
add_subdirectory(src/force)
add_subdirectory(src/circular)
add_subdirectory(src/batch)

option(GRAFFODIL_BUILD_DEMO  "Build the demo app"   ${PROJECT_IS_TOP_LEVEL})
option(GRAFFODIL_BUILD_TESTS "Build the test suite" ${PROJECT_IS_TOP_LEVEL})

if(GRAFFODIL_BUILD_DEMO)
    add_subdirectory(demo)
endif()

if(GRAFFODIL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
