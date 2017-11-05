
FILE(GLOB_RECURSE HeaderFiles "*.hpp" "*.h")
add_custom_target(headers SOURCES ${HeaderFiles})

add_library(shmufbuf INTERFACE)
target_include_directories(shmufbuf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(shmufbuf INTERFACE rt)

add_library(bufbuf INTERFACE)
target_include_directories(bufbuf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bufbuf INTERFACE pthread)

add_library(scratchpad INTERFACE) #
target_include_directories(scratchpad INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(scratchpad INTERFACE cxx_std_17)

add_executable(ipc_unix_sockets ipc_unix_sockets.cpp)

add_executable(tmpl_params_deduction constructor_tmpl_param_deduction.cpp)
target_compile_features(tmpl_params_deduction PRIVATE cxx_std_17)
