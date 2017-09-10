# /bench cmake file
if(NOT CPP_PLAYGROUND_BUILD_BENCH)
   return()
endif()

find_package(benchmark REQUIRED)

add_executable(bench_vec_erase_idx vec_erase_idx_b.cpp)
target_link_libraries(bench_vec_erase_idx PRIVATE benchmark::benchmark vec_idx_erase)
