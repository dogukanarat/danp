# ==============================================================================
# FindZeroMQ.cmake
# ==============================================================================

# 1. Try to find PkgConfig to get hints for the search
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_ZMQ QUIET libzmq)
endif()

# 2. Search for the header file
find_path(ZeroMQ_INCLUDE_DIR
    NAMES zmq.h
    HINTS 
        ${PC_ZMQ_INCLUDE_DIRS}
        ${ZeroMQ_ROOT}
        $ENV{ZeroMQ_ROOT}
    PATH_SUFFIXES include
)

# 3. Search for the library file
find_library(ZeroMQ_LIBRARY
    NAMES zmq
    HINTS 
        ${PC_ZMQ_LIBRARY_DIRS}
        ${ZeroMQ_ROOT}
        $ENV{ZeroMQ_ROOT}
    PATH_SUFFIXES lib lib64 bin
)

# 4. Handle REQUIRED, QUIET and versioning
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZeroMQ
    REQUIRED_VARS ZeroMQ_LIBRARY ZeroMQ_INCLUDE_DIR
    VERSION_VAR PC_ZMQ_VERSION
)

# 5. Create the IMPORTED target for Modern CMake
if(ZeroMQ_FOUND AND NOT TARGET ZeroMQ::ZeroMQ)
    add_library(ZeroMQ::ZeroMQ UNKNOWN IMPORTED)
    set_target_properties(ZeroMQ::ZeroMQ PROPERTIES
        IMPORTED_LOCATION "${ZeroMQ_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ZeroMQ_INCLUDE_DIR}"
    )
    
    # On Windows, ZeroMQ often requires the Ws2_32 (Winsock) library
    if(WIN32)
        set_target_properties(ZeroMQ::ZeroMQ PROPERTIES
            INTERFACE_LINK_LIBRARIES "Ws2_32"
        )
    endif()
endif()

# Clean up the cache from internal variables
mark_as_advanced(ZeroMQ_INCLUDE_DIR ZeroMQ_LIBRARY)