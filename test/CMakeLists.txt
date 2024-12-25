cmake_minimum_required(VERSION 3.10)

include(GoogleTest)

file(GLOB_RECURSE VECTORDB_TEST_SOURCES "${PROJECT_SOURCE_DIR}/test/*/*test.cpp")

# #####################################################################################################################
# MAKE TARGETS
# #####################################################################################################################

# #########################################
# "make check-tests"
# #########################################
add_custom_target(build-tests COMMAND ${CMAKE_CTEST_COMMAND} --show-only)
add_custom_target(check-tests COMMAND ${CMAKE_CTEST_COMMAND} --verbose)


# #########################################
# "make XYZ_test"
# #########################################
foreach (vectordb_test_source ${VECTORDB_TEST_SOURCES})
    # Create a human readable name.
    get_filename_component(vectordb_test_filename ${vectordb_test_source} NAME)
    string(REPLACE ".cpp" "" vectordb_test_name ${vectordb_test_filename})

    # Add the test target separately and as part of "make check-tests".
    #add_executable(${vectordb_test_name} EXCLUDE_FROM_ALL ${vectordb_test_source})
    add_executable(${vectordb_test_name} EXCLUDE_FROM_ALL ${vectordb_test_source} "${PROJECT_SOURCE_DIR}/tools/backtrace.cpp")
    add_backward(${vectordb_test_name})
    add_dependencies(build-tests ${vectordb_test_name})
    add_dependencies(check-tests ${vectordb_test_name})

    gtest_discover_tests(${vectordb_test_name}
            EXTRA_ARGS
            --gtest_output=xml:${CMAKE_BINARY_DIR}/test/${vectordb_test_name}.xml
            --gtest_catch_exceptions=0
            DISCOVERY_TIMEOUT 120
            PROPERTIES
            TIMEOUT 120
            )
         
    target_link_libraries(${vectordb_test_name} vectorDB gtest gmock_main)

    # Set test target properties and dependencies.
    set_target_properties(${vectordb_test_name}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
            COMMAND ${vectordb_test_name}
            )
endforeach ()


