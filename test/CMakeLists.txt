# ============================================================================
# Test Suite Configuration
# ============================================================================

# Fetch Unity testing framework
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)

# ============================================================================
# Test Helper Function
# ============================================================================
function(danp_add_test TEST_NAME)
    cmake_parse_arguments(
        ARG
        ""
        "SOURCE"
        "ADDITIONAL_SOURCES"
        ${ARGN}
    )
    
    # Create test executable
    add_executable(${TEST_NAME}
        ${ARG_SOURCE}
        ${ARG_ADDITIONAL_SOURCES}
        mock_driver.c
        log_message.c
    )
    
    # Link against library and dependencies
    target_link_libraries(${TEST_NAME}
        PRIVATE
            danp::danp
            unity
            Threads::Threads
    )
    
    # Add include directory for test-specific headers
    target_include_directories(${TEST_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # Register with CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    
    # Set test properties
    set_tests_properties(${TEST_NAME} PROPERTIES
        TIMEOUT 30
        LABELS "unit"
    )
endfunction()

# ============================================================================
# Test Targets
# ============================================================================
danp_add_test(test_core SOURCE test_core.c)
danp_add_test(test_dgram SOURCE test_dgram.c)
danp_add_test(test_stream SOURCE test_stream.c)

# ============================================================================
# Code Coverage Target
# ============================================================================
if(ENABLE_COVERAGE)
    include(CodeCoverage)

    # Setup coverage target that runs all tests
    setup_target_for_coverage_lcov(
        NAME coverage
        EXECUTABLE ctest
        EXECUTABLE_ARGS --output-on-failure
        DEPENDENCIES test_core test_dgram test_stream
        EXCLUDE
            'test/*'
            'unity/*'
            '*/mock_driver.c'
            '*/log_message.c'
    )

    message(STATUS "Coverage target 'coverage' configured")
    message(STATUS "Run 'cmake --build . --target coverage' to generate coverage report")
endif()

# ============================================================================
# Test Summary
# ============================================================================
message(STATUS "Test suite configured:")
message(STATUS "  - test_core: Core functionality tests")
message(STATUS "  - test_dgram: DGRAM socket tests")
message(STATUS "  - test_stream: STREAM socket tests")
message(STATUS "Run 'ctest' or 'cmake --build . --target test' after building")