project(cpp_playground)
cmake_minimum_required(VERSION 3.8)

option(CPP_PLAYGROUND_BUILD_BENCH "Build benchmarks" ON)
option(CPP_PLAYGROUND_BUILD_TESTS "Build unittests." ON)
option(CPP_PLAYGROUND_BUILD_DOC "Build project documentation" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/config")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(GCC_ALL_WARNINGS_FLAGS "-Wall -Wextra -pedantic")
set(CLANG_FALSE_WARNING_FLAGS "-Wno-gnu-statement-expression -Wno-missing-braces")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_ALL_WARNINGS_FLAGS} ${CLANG_FALSE_WARNING_FLAGS}")

enable_testing()

add_subdirectory(bench)
add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(test)
