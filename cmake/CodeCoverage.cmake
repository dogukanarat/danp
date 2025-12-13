# ============================================================================
# Code Coverage Configuration Module
# ============================================================================
# This module provides functions and configuration for code coverage analysis
# using gcov/lcov for GCC and Clang compilers.
#
# Usage:
#   include(CodeCoverage)
#   append_coverage_compiler_flags()
#   setup_target_for_coverage_lcov(
#       NAME coverage
#       EXECUTABLE ctest
#       DEPENDENCIES test_target
#   )

include(CMakeParseArguments)

# Check prerequisites
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)

if(NOT LCOV_PATH)
    message(STATUS "lcov not found! Coverage reports will not be available.")
    message(STATUS "Install lcov: sudo apt-get install lcov (Ubuntu/Debian)")
endif()

if(NOT GENHTML_PATH)
    message(STATUS "genhtml not found! HTML coverage reports will not be available.")
endif()

# Coverage compiler flags
set(COVERAGE_COMPILER_FLAGS "-g -O0 --coverage -fprofile-arcs -ftest-coverage"
    CACHE STRING "Flags used by the C compiler during coverage builds."
    FORCE
)

set(COVERAGE_LINKER_FLAGS "--coverage"
    CACHE STRING "Flags used for linking binaries during coverage builds."
    FORCE
)

mark_as_advanced(
    COVERAGE_COMPILER_FLAGS
    COVERAGE_LINKER_FLAGS
)

# Function to append coverage compiler flags to CMAKE_C_FLAGS
function(append_coverage_compiler_flags)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COVERAGE_COMPILER_FLAGS}" PARENT_SCOPE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COVERAGE_LINKER_FLAGS}" PARENT_SCOPE)
        message(STATUS "Appending code coverage compiler flags: ${COVERAGE_COMPILER_FLAGS}")
    else()
        message(WARNING "Code coverage only supported with GCC or Clang compilers!")
    endif()
endfunction()

# Function to add coverage compiler flags to a specific target
function(append_coverage_compiler_flags_to_target target)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE ${COVERAGE_COMPILER_FLAGS})
        target_link_options(${target} PRIVATE ${COVERAGE_LINKER_FLAGS})
    else()
        message(WARNING "Code coverage only supported with GCC or Clang compilers!")
    endif()
endfunction()

# Setup target for coverage using lcov
# Creates a custom target that runs tests and generates coverage reports
#
# Parameters:
#   NAME           - Name of the coverage target (e.g., 'coverage')
#   EXECUTABLE     - The test executable or command to run (e.g., 'ctest')
#   EXECUTABLE_ARGS - Arguments to pass to the executable (optional)
#   DEPENDENCIES   - Targets that must be built before running coverage (optional)
#   EXCLUDE        - Patterns to exclude from coverage report (optional)
function(setup_target_for_coverage_lcov)
    set(options NONE)
    set(oneValueArgs NAME EXECUTABLE)
    set(multiValueArgs EXECUTABLE_ARGS DEPENDENCIES EXCLUDE)

    cmake_parse_arguments(
        Coverage
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    if(NOT LCOV_PATH)
        message(WARNING "lcov not found! Target ${Coverage_NAME} will not be created.")
        return()
    endif()

    if(NOT GENHTML_PATH)
        message(WARNING "genhtml not found! HTML report generation will be skipped.")
    endif()

    # Default excludes
    set(COVERAGE_EXCLUDES
        '/usr/*'
        '${CMAKE_BINARY_DIR}/*'
        '*/test/*'
        '*/unity/*'
        '*/_deps/*'
        ${Coverage_EXCLUDE}
    )

    # Setup coverage target
    add_custom_target(${Coverage_NAME}
        # Cleanup lcov
        COMMAND ${LCOV_PATH} --directory . --zerocounters

        # Create baseline coverage data
        COMMAND ${LCOV_PATH} --capture --initial --directory . --output-file ${Coverage_NAME}.base

        # Run tests
        COMMAND ${Coverage_EXECUTABLE} ${Coverage_EXECUTABLE_ARGS}

        # Capture coverage data
        COMMAND ${LCOV_PATH} --directory . --capture --output-file ${Coverage_NAME}.info

        # Combine baseline and test coverage data
        COMMAND ${LCOV_PATH} --add-tracefile ${Coverage_NAME}.base --add-tracefile ${Coverage_NAME}.info --output-file ${Coverage_NAME}.total

        # Filter out unwanted files
        COMMAND ${LCOV_PATH} --remove ${Coverage_NAME}.total ${COVERAGE_EXCLUDES} --output-file ${Coverage_NAME}.info.cleaned

        # Generate HTML report if genhtml is available
        COMMAND ${CMAKE_COMMAND} -E $<IF:$<BOOL:${GENHTML_PATH}>,echo,true> "Generating HTML coverage report..."
        COMMAND ${GENHTML_PATH} $<$<BOOL:${GENHTML_PATH}>:${Coverage_NAME}.info.cleaned>
                $<$<BOOL:${GENHTML_PATH}>:--output-directory> $<$<BOOL:${GENHTML_PATH}>:${Coverage_NAME}_html>
                $<$<BOOL:${GENHTML_PATH}>:--demangle-cpp>
                $<$<BOOL:${GENHTML_PATH}>:--title> $<$<BOOL:${GENHTML_PATH}>:"${PROJECT_NAME} Coverage Report">
                $<$<BOOL:${GENHTML_PATH}>:--legend>

        # Display summary
        COMMAND ${LCOV_PATH} --list ${Coverage_NAME}.info.cleaned

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${Coverage_DEPENDENCIES}
        COMMENT "Running coverage analysis..."
    )

    # Show message about report location
    add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated!"
        COMMAND ${CMAKE_COMMAND} -E echo "Text summary: ${CMAKE_BINARY_DIR}/${Coverage_NAME}.info.cleaned"
        COMMAND ${CMAKE_COMMAND} -E $<IF:$<BOOL:${GENHTML_PATH}>,echo,true> "HTML report: ${CMAKE_BINARY_DIR}/${Coverage_NAME}_html/index.html"
    )

    # Clean coverage files
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES
        "${CMAKE_BINARY_DIR}/${Coverage_NAME}.base"
        "${CMAKE_BINARY_DIR}/${Coverage_NAME}.info"
        "${CMAKE_BINARY_DIR}/${Coverage_NAME}.total"
        "${CMAKE_BINARY_DIR}/${Coverage_NAME}.info.cleaned"
        "${CMAKE_BINARY_DIR}/${Coverage_NAME}_html"
    )

endfunction()
