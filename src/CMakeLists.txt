
FILE(GLOB_RECURSE HeaderFiles "*.hpp" "*.h")
add_custom_target(headers SOURCES ${HeaderFiles})

add_library(vec_idx_erase INTERFACE)
target_include_directories(vec_idx_erase INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
# target_sources(vec_idx_erase INTERFACE vector_erase_indexes.hpp)
