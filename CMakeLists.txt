# ==============================================================================
# Project Definition
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GitVersion)

get_git_version(PROJECT_SEMVER PROJECT_GIT_STRING)

project(Danp
    VERSION ${PROJECT_SEMVER}
    DESCRIPTION "A scaffold library for CMake projects"
    LANGUAGES C
)

# ==============================================================================
# CMake Modules and Policies
# ==============================================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

# ==============================================================================
# Build Options
# ==============================================================================

option(BUILD_SHARED_LIBS "Build shared libraries instead of static" ON)
option(BUILD_TESTS "Build unit tests with Unity framework" ON)
option(BUILD_EXAMPLES "Build example applications" ON)
option(DANP_SUPPORT_ZMQ "Enable ZeroMQ support in Danp library" ON)

# ==============================================================================
# Project Configuration
# ==============================================================================

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
    )
endif()

# ==============================================================================
# File Generation (Config & Version)
# ==============================================================================

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/danp/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/danp/config.h
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/danp/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/danp/version.h
    @ONLY
)

# ==============================================================================
# Library Target Definition
# ==============================================================================

add_library(${PROJECT_NAME} "")
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/danp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/danp_int.c
)

# ==============================================================================
# Library Include Directories
# ==============================================================================

target_include_directories(${PROJECT_NAME}
    PUBLIC
        # 1. Source headers
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        # 2. Generated headers (Critical Fix!)
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        # 3. Installed headers
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ==============================================================================
# Library Properties
# ==============================================================================

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        OUTPUT_NAME "danp"
)

# ==============================================================================
# Compiler Features and Options
# ==============================================================================

target_compile_features(${PROJECT_NAME} PUBLIC c_std_99)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>>:
            -Wall -Wextra -Wpedantic
        >
        $<$<C_COMPILER_ID:MSVC>:
            /W4
        >
)

# ==============================================================================
# Optional Components
# ==============================================================================

if(BUILD_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
        enable_testing()
        add_subdirectory(test)
    else()
        message(WARNING "BUILD_TESTS is ON but 'test' directory is missing.")
    endif()
endif()

if (BUILD_EXAMPLES)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
        add_subdirectory(examples)
    else()
        message(WARNING "BUILD_EXAMPLES is ON but 'examples' directory is missing.")
    endif()
endif()

# ==============================================================================
# Installation Rules
# ==============================================================================

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Runtime NAMELINK_COMPONENT Development
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install static headers from source
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/danp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT Development
    FILES_MATCHING PATTERN "*.h"
)

# Install generated headers from binary dir (Critical Fix!)
install(
    FILES 
        ${CMAKE_CURRENT_BINARY_DIR}/include/danp/config.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/danp/version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/danp
    COMPONENT Development
)

# ==============================================================================
# CMake Package Configuration
# ==============================================================================

install(
    EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    COMPONENT Development
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    COMPONENT Development
)

# ==============================================================================
# pkg-config Support
# ==============================================================================

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
    @ONLY
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    COMPONENT Development
)

# ==============================================================================
# Summary
# ==============================================================================

message(STATUS "")
message(STATUS "==================================================")
message(STATUS "  ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "==================================================")
message(STATUS "  BUILD_TYPE:              ${CMAKE_BUILD_TYPE}")
message(STATUS "  C_COMPILER:              ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  BUILD_SHARED_LIBS:       ${BUILD_SHARED_LIBS}")
message(STATUS "  BUILD_TESTS:             ${BUILD_TESTS}")
message(STATUS "  BUILD_EXAMPLES:          ${BUILD_EXAMPLES}")
message(STATUS "  CMAKE_INSTALL_PREFIX:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  PROJECT_SEMVER:          ${PROJECT_SEMVER}")
message(STATUS "  PROJECT_GIT_STRING:      ${PROJECT_GIT_STRING}")
message(STATUS "==================================================")
message(STATUS "")
