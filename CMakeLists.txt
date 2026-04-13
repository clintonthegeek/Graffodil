cmake_minimum_required(VERSION 3.19)
project(Graffodil VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Widgets Test)

add_subdirectory(src/core)
add_subdirectory(src/sugiyama)
add_subdirectory(src/force)
add_subdirectory(src/circular)
add_subdirectory(src/batch)

enable_testing()
add_subdirectory(tests)
