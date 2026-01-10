# ==============================================================================
# Project Definition
# ==============================================================================

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# TODO: Update the project name and description after running scripts/migration.sh.
project(Danp
    VERSION 1.0.0
    DESCRIPTION "A scaffold library for CMake projects"
    LANGUAGES C
)

# ==============================================================================
# CMake Modules and Policies
# ==============================================================================

# Include standard CMake modules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Set custom module path for additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# Export compile commands for IDE integration (clangd, ccls, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

# ==============================================================================
# Build Options
# ==============================================================================

# Options to control build configuration
option(BUILD_SHARED_LIBS "Build shared libraries instead of static" OFF)
option(BUILD_TESTS "Build unit tests with Unity framework" ON)
option(BUILD_EXAMPLES "Build example applications" ON)
option(DANP_SUPPORT_ZMQ "Enable ZeroMQ support in Danp library" ON)

# ==============================================================================
# Project Configuration
# ==============================================================================

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
    # Set possible build type values for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
    )
endif()

# ==============================================================================
# Library Target Definition
# ==============================================================================

# Create the main library target (static or shared based on BUILD_SHARED_LIBS)
add_library(Danp)

# Create an alias for consistent namespacing (allows using Danp::Danp)
add_library(Danp::Danp ALIAS Danp)
add_library(danp ALIAS Danp)

# Add source files to the library target
target_sources(Danp
    PRIVATE
        # Core implementation files
        ${CMAKE_CURRENT_SOURCE_DIR}/src/danp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/danp_buffer.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/danp_route.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/danp_socket.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/danp_zerocopy.c
        $<$<BOOL:${DANP_SUPPORT_ZMQ}>:${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/danp_zmq.c>
)

# ==============================================================================
# Library Include Directories
# ==============================================================================

# Configure include directories for the library
# PUBLIC: Propagated to targets linking against this library
# BUILD_INTERFACE: Used when building the library
# INSTALL_INTERFACE: Used when library is installed
target_include_directories(Danp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ==============================================================================
# Compile Definitions
# ==============================================================================

set(_COMPILE_DEFINITIONS "")

# Add compile definitions based on build type

# ==============================================================================
# Compiler Features and Options
# ==============================================================================

# Require C99 standard (public requirement propagates to consumers)
target_compile_features(Danp PUBLIC c_std_99)

# Add compiler warnings (private - only when building this library)
target_compile_options(Danp
    PRIVATE
        # GCC and Clang warnings
        $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>>:
            -Wall           # Enable common warnings
            -Wextra         # Enable extra warnings
            -Wpedantic      # Strict ISO C compliance warnings
        >
        # MSVC warnings
        $<$<C_COMPILER_ID:MSVC>:
            /W4             # Warning level 4
        >
)

list(APPEND _COMPILE_DEFINITIONS
    SCAFFOLD_PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    SCAFFOLD_PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    SCAFFOLD_PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

if(BUILD_SHARED_LIBS)
    list(APPEND _COMPILE_DEFINITIONS SCAFFOLD_PROJECT_SHARED)
else()
    list(APPEND _COMPILE_DEFINITIONS SCAFFOLD_PROJECT_STATIC)
endif()

target_compile_definitions(Danp
    PRIVATE
        ${_COMPILE_DEFINITIONS}
)

# ==============================================================================
# Library Properties
# ==============================================================================

# Set library version and symbol visibility
set_target_properties(Danp
    PROPERTIES
        # Library versioning (for shared libraries)
        VERSION ${PROJECT_VERSION}                  # Full version (e.g., 1.0.0)
        SOVERSION ${PROJECT_VERSION_MAJOR}          # Major version (e.g., 1)

        # Symbol visibility (hide internal symbols in shared libraries)
        C_VISIBILITY_PRESET hidden                  # Hide symbols by default
        VISIBILITY_INLINES_HIDDEN YES               # Hide inline functions

        # Output name (can be customized if needed)
        OUTPUT_NAME "danp"
)

# ==============================================================================
# Dependencies
# ==============================================================================

# Find and link external dependencies required by the library
# Uncomment and modify as needed for your project

set(_DEPS_PUBLIC "")
set(_DEPS_PRIVATE "")

#Find and link OSAL library
find_package(Osal REQUIRED)
if (Osal_FOUND)
    list(APPEND _DEPS_PUBLIC osal)
else()
    message(FATAL_ERROR "Osal library not found")
endif()

# Link dependencies to the library target
target_link_libraries(Danp
    PUBLIC
        ${_DEPS_PUBLIC}
    PRIVATE
        ${_DEPS_PRIVATE}
)

# ==============================================================================
# Optional Components
# ==============================================================================

# Enable testing if BUILD_TESTS is ON
if(BUILD_TESTS)
    message(STATUS "Building tests enabled")
    enable_testing()
    add_subdirectory(test)
endif()

# Add example subdirectory if BUILD_EXAMPLES is ON
if(BUILD_EXAMPLES)
    message(STATUS "Building examples enabled")
    add_subdirectory(example)
endif()

# ==============================================================================
# Installation Rules
# ==============================================================================

# Install the library target
# Components: Runtime (shared libs), Development (static libs, headers)
install(
    TARGETS Danp
    EXPORT DanpTargets

    # Shared libraries go to lib/ (Runtime component)
    LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT Runtime
        NAMELINK_COMPONENT Development

    # Static libraries go to lib/ (Development component)
    ARCHIVE
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT Development

    # Windows DLLs go to bin/ (Runtime component)
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime

    # Public headers are in the include directory
    INCLUDES
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install public header files
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/danp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT Development
    FILES_MATCHING PATTERN "*.h"
)

# ==============================================================================
# CMake Package Configuration
# ==============================================================================

# Install CMake targets file (allows find_package to work)
install(
    EXPORT DanpTargets
    FILE DanpTargets.cmake
    NAMESPACE Danp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Danp
    COMPONENT Development
)

# Generate package config file from template
# This file is used by find_package(Danp)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DanpConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/DanpConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Danp
)

# Generate package version file
# Ensures version compatibility when using find_package
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/DanpConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion  # 1.x.x is compatible with 1.y.z
)

# Install the generated config files
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/DanpConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/DanpConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Danp
    COMPONENT Development
)

# ==============================================================================
# pkg-config Support
# ==============================================================================

# Generate and install pkg-config file for projects using pkg-config
# instead of CMake's find_package

# Generate the .pc file from template
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Danp.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/Danp.pc
    @ONLY
)

# Install the pkg-config file
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/Danp.pc
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
message(STATUS "==================================================")
message(STATUS "")
