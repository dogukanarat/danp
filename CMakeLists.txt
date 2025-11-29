# ============================================================================
# Project Configuration
# ============================================================================
cmake_minimum_required(VERSION 3.14)

project(danp
    VERSION 1.0.0
    DESCRIPTION "Lightweight network protocol library for embedded systems"
    LANGUAGES C
)

# ============================================================================
# Options
# ============================================================================
option(DANP_ARCH_POSIX "Enable POSIX architecture support" ON)
option(DANP_ARCH_FREERTOS "Enable FreeRTOS architecture support" OFF)
option(DANP_ZMQ_SUPPORT "Enable ZeroMQ driver support" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTS "Build test suite" OFF)
option(BUILD_EXAMPLES "Build example applications" OFF)
option(INSTALL_DOCS "Install documentation" ON)

# ============================================================================
# CMake Settings
# ============================================================================
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# Dependencies
# ============================================================================
find_package(Threads REQUIRED)
find_package(osal REQUIRED)

if(DANP_ZMQ_SUPPORT)
    find_package(ZeroMQ REQUIRED)
    if(NOT ZeroMQ_FOUND)
        message(FATAL_ERROR "ZeroMQ support requested but not found.")
    endif()
endif()

# ============================================================================
# Library Target
# ============================================================================
add_library(danp)
add_library(danp::danp ALIAS danp)

# Core sources
target_sources(danp
    PRIVATE
        src/danp.c
        src/danpSocket.c
)

if(DANP_ARCH_FREERTOS)
    target_sources(danp PRIVATE src/arch/danpFreeRTOS.c)
endif()

# Driver sources
if(DANP_ZMQ_SUPPORT)
    target_sources(danp PRIVATE src/driver/danpZmq.c)
endif()

# ============================================================================
# Target Properties
# ============================================================================

# Include directories
target_include_directories(danp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link libraries
target_link_libraries(danp
    PUBLIC
        Threads::Threads
        osal::osal
)

if(DANP_ZMQ_SUPPORT)
    target_link_libraries(danp PUBLIC ${ZeroMQ_LIBRARIES})
    target_include_directories(danp PUBLIC ${ZeroMQ_INCLUDE_DIRS})
endif()

# Compile features
target_compile_features(danp PUBLIC c_std_99)

# Compile options
target_compile_options(danp
    PRIVATE
        $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<C_COMPILER_ID:MSVC>:/W4>
)

# Compile definitions
if(DANP_ARCH_POSIX)
    target_compile_definitions(danp PUBLIC DANP_ARCH_POSIX)
endif()

if(DANP_ARCH_FREERTOS)
    target_compile_definitions(danp PUBLIC DANP_ARCH_FREERTOS)
endif()

if(DANP_ZMQ_SUPPORT)
    target_compile_definitions(danp PUBLIC DANP_ZMQ_SUPPORT)
endif()

# Set library properties
set_target_properties(danp PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

# ============================================================================
# Subdirectories
# ============================================================================
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

# ============================================================================
# Installation
# ============================================================================
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install library
install(TARGETS danp
    EXPORT danpTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT Runtime
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT Development
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install public headers
install(DIRECTORY include/danp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT Development
    FILES_MATCHING PATTERN "*.h"
)

# Install CMake targets
install(EXPORT danpTargets
    FILE danpTargets.cmake
    NAMESPACE danp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/danp
    COMPONENT Development
)

# Generate and install package config files
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/danpConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/danpConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/danp
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/danpConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/danpConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/danpConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/danp
    COMPONENT Development
)

# Install documentation
if(INSTALL_DOCS)
    install(FILES
        README.md
        SPHINX_GUIDE.md
        INSTALL.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
        COMPONENT Documentation
        OPTIONAL
    )
    
    install(DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/html
        ${CMAKE_CURRENT_SOURCE_DIR}/sphinx
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
        COMPONENT Documentation
        OPTIONAL
    )
endif()

# ============================================================================
# Package Configuration (Optional)
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    
    include(CPack)
endif()