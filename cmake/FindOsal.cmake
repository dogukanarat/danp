# ==============================================================================
# FindOsal.cmake
# ==============================================================================

# 1. Use PkgConfig as a hint (if available)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_OSAL QUIET osal)
endif()

# 2. Find the header file
find_path(Osal_INCLUDE_DIR
    NAMES osal/osal_thread.h
    HINTS 
        ${PC_OSAL_INCLUDE_DIRS} 
        ${Osal_ROOT} 
        ${OSAL_DIR}
    PATH_SUFFIXES include
)

# 3. Find the library file
find_library(Osal_LIBRARY
    NAMES osal
    HINTS 
        ${PC_OSAL_LIBRARY_DIRS} 
        ${Osal_ROOT} 
        ${OSAL_DIR}
    PATH_SUFFIXES lib bin
)

# 4. Handle REQUIRED and versioning
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Osal
    REQUIRED_VARS Osal_LIBRARY Osal_INCLUDE_DIR
)

# 5. Create the IMPORTED target
if(Osal_FOUND AND NOT TARGET osal::osal)
    add_library(osal::osal UNKNOWN IMPORTED)
    set_target_properties(osal::osal PROPERTIES
        IMPORTED_LOCATION "${Osal_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Osal_INCLUDE_DIR}"
    )
endif()

# Hide internal variables from the CMake cache
mark_as_advanced(Osal_INCLUDE_DIR Osal_LIBRARY)