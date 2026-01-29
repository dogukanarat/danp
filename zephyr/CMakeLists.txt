# Copyright (c) 2025 Plan-S Satellite and Space Technologies Inc.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_DANP)

#------------------------------------------------------------------------------
# Paths
#------------------------------------------------------------------------------
set(_DANP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(_DANP_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

#------------------------------------------------------------------------------
# Library
#------------------------------------------------------------------------------
zephyr_library_named(danp)

#------------------------------------------------------------------------------
# Generated Config Header
#------------------------------------------------------------------------------
file(MAKE_DIRECTORY ${_DANP_GEN_DIR}/danp)

configure_file(
    ${_DANP_ROOT}/include/danp/config.h.in
    ${_DANP_GEN_DIR}/danp/config.h
    @ONLY
)

#------------------------------------------------------------------------------
# Include Directories
#------------------------------------------------------------------------------
# Public (visible to dependents)
zephyr_include_directories(
    ${_DANP_ROOT}/src
    ${_DANP_ROOT}/include
    ${_DANP_GEN_DIR}
)

# Private (only for danp compilation)
zephyr_library_include_directories(
    ${_DANP_ROOT}/src
)

#------------------------------------------------------------------------------
# Core Sources
#------------------------------------------------------------------------------
zephyr_library_sources(
    ${_DANP_ROOT}/src/danp.c
    ${_DANP_ROOT}/src/danp_socket.c
    ${_DANP_ROOT}/src/danp_buffer.c
    ${_DANP_ROOT}/src/danp_route.c
    ${_DANP_ROOT}/src/danp_zerocopy.c
    ${_DANP_ROOT}/src/drivers/danp_lo.c
    ${_DANP_ROOT}/src/drivers/danp_z_radio.c
)

#------------------------------------------------------------------------------
# Optional Features
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Compile Definitions
#------------------------------------------------------------------------------
zephyr_library_compile_definitions(
    DANP_ZEPHYR=1
)

zephyr_library_compile_definitions_ifdef(CONFIG_DANP_ZEROCOPY
    DANP_ZEROCOPY_ENABLED=1
)

zephyr_library_compile_definitions_ifdef(CONFIG_DANP_DEBUG
    DANP_DEBUG=1
)

# Pass Kconfig values to C code
if(DEFINED CONFIG_DANP_MAX_SOCKETS)
    zephyr_library_compile_definitions(
        DANP_MAX_SOCKETS=${CONFIG_DANP_MAX_SOCKETS}
    )
endif()

if(DEFINED CONFIG_DANP_BUFFER_SIZE)
    zephyr_library_compile_definitions(
        DANP_BUFFER_SIZE=${CONFIG_DANP_BUFFER_SIZE}
    )
endif()

if(DEFINED CONFIG_DANP_LOG_LEVEL)
    zephyr_library_compile_definitions(
        DANP_LOG_LEVEL=${CONFIG_DANP_LOG_LEVEL}
    )
endif()

#------------------------------------------------------------------------------
# Dependencies
#------------------------------------------------------------------------------
zephyr_library_link_libraries(osal)

endif() # CONFIG_DANP