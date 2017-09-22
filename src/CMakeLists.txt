
FILE(GLOB_RECURSE HeaderFiles "*.hpp" "*.h")
add_custom_target(headers SOURCES ${HeaderFiles})

add_library(vec_idx_erase INTERFACE)
target_include_directories(vec_idx_erase INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(frame_ipc frame_ipc.cpp)
add_executable(udp_ipc_2 udp_ipc_server_sends.cpp)
