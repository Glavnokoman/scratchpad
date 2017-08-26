# /bench cmake file

if(NOT CPP_PLAYGROUND_BUILD_BENCH)
   return()
endif()

find_package(benchmark REQUIRED)

add_executable(bench_example example_b.cpp)
target_link_libraries(bench_example PRIVATE benchmark::benchmark)
