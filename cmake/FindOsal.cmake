# FindOsal.cmake - Locate the OSAL library
# This module defines the following variables:
#   OSAL_FOUND        - True if OSAL is found
#   OSAL_INCLUDE_DIRS - Include directories for OSAL
#   OSAL_LIBRARIES    - Libraries to link against OSAL

find_path(OSAL_INCLUDE_DIR
    NAMES osal/osal_thread.h
    PATHS ${CMAKE_INSTALL_PREFIX}/include /usr/include
)

find_library(OSAL_LIBRARY
    NAMES osal
    PATHS ${CMAKE_INSTALL_PREFIX}/lib /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Osal
    REQUIRED_VARS OSAL_INCLUDE_DIR OSAL_LIBRARY
    FAIL_MESSAGE "Could not find OSAL library"
)

if(OSAL_FOUND)
    set(OSAL_INCLUDE_DIRS ${OSAL_INCLUDE_DIR})
    set(OSAL_LIBRARIES ${OSAL_LIBRARY})

    # Create an imported target for OSAL
    add_library(osal::osal UNKNOWN IMPORTED)
    set_target_properties(osal::osal PROPERTIES
        IMPORTED_LOCATION ${OSAL_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${OSAL_INCLUDE_DIRS}
    )
endif()