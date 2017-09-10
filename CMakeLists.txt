project(cpp_playground)
cmake_minimum_required(VERSION 3.8)

option(CPP_PLAYGROUND_BUILD_BENCH "Build benchmarks" ON)
option(CPP_PLAYGROUND_BUILD_TESTS "Build unittests." ON)
option(CPP_PLAYGROUND_BUILD_DOC "Build project documentation" OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/config")
enable_testing()

add_subdirectory(bench)
add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(test)
