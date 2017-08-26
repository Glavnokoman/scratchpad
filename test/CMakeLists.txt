# /test cmake file

if(NOT CPP_PLAYGROUND_BUILD_TESTS)
   return()
endif()

find_package(Catch REQUIRED)

function(add_catch_test arg_test_name arg_test_src)
	add_executable(${arg_test_name} ${arg_test_src})
	# target_link_libraries(${arg_test_name} PRIVATE ${CATCH_LIBRARIES})
   target_include_directories( ${arg_test_name} PRIVATE ${CATCH_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/src )
	add_test(NAME ${arg_test_name} COMMAND ${arg_test_name} )
endfunction()

add_catch_test(test_example example_t.cpp)
