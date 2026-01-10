# ==============================================================================
# Test Suite Configuration
# ==============================================================================

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# ==============================================================================
# Test Options
# ==============================================================================

option(BUILD_TESTS_WITH_LOGS "Enable logging output during tests" ON)

# ==============================================================================
# Test Dependencies
# ==============================================================================

find_package(Danp REQUIRED)
if (NOT Danp_FOUND)
    message(FATAL_ERROR "Danp library not found. Please install Danp or set Danp_DIR.")
endif()

find_package(Unity REQUIRED)
if (NOT UNITY_FOUND AND NOT Unity_FOUND)
    message(STATUS "Unity not found via find_package. Using FetchContent to download Unity.")
endif()

# ==============================================================================
# Helper Function to Create Test Executables
# ==============================================================================

function(add_danp_test TEST_NAME TEST_SOURCE)
    set(_TEST_SOURCES ${TEST_SOURCE})

    if(BUILD_TESTS_WITH_LOGS)
        list(APPEND _TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/log_message.c)
    endif()

    add_executable(${TEST_NAME} ${_TEST_SOURCES})

    set(_COMPILE_DEFINITIONS_PRIVATE "")
    if(BUILD_TESTS_WITH_LOGS)
        list(APPEND _COMPILE_DEFINITIONS_PRIVATE "BUILD_TESTS_WITH_LOGS")
    endif()

    target_compile_definitions(${TEST_NAME}
        PRIVATE
            ${_COMPILE_DEFINITIONS_PRIVATE}
    )

    target_link_libraries(${TEST_NAME}
        PRIVATE
            danp
            unity
    )

    if(NOT UNITY_FOUND AND NOT Unity_FOUND)
        target_include_directories(${TEST_NAME}
            PRIVATE
                ${unity_SOURCE_DIR}/src
        )
    endif()

    add_test(
        NAME ${TEST_NAME}
        COMMAND ${TEST_NAME}
    )

    set_tests_properties(${TEST_NAME}
        PROPERTIES
            TIMEOUT 30
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            LABELS "unit;danp"
    )
endfunction()

# ==============================================================================
# Test Executable Definitions
# ==============================================================================

add_danp_test(test_core ${CMAKE_CURRENT_SOURCE_DIR}/test_core.c)
add_danp_test(test_dgram ${CMAKE_CURRENT_SOURCE_DIR}/test_dgram.c)
add_danp_test(test_route ${CMAKE_CURRENT_SOURCE_DIR}/test_route.c)
add_danp_test(test_stream ${CMAKE_CURRENT_SOURCE_DIR}/test_stream.c)
add_danp_test(test_zerocopy ${CMAKE_CURRENT_SOURCE_DIR}/test_zerocopy.c)

# ==============================================================================
# Summary
# ==============================================================================

message(STATUS "Test suite configured successfully")
message(STATUS "  Tests: test_core, test_dgram, test_route, test_stream, test_zerocopy")
if(UNITY_FOUND OR Unity_FOUND)
    message(STATUS "  Unity source: System installation")
else()
    message(STATUS "  Unity source: FetchContent (downloaded)")
endif()
