# ==============================================================================
# FindUnity.cmake
# ==============================================================================

find_path(Unity_INCLUDE_DIR
    NAMES unity.h
    PATH_SUFFIXES src unity
)

find_library(Unity_LIBRARY
    NAMES unity
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Unity
    REQUIRED_VARS Unity_LIBRARY Unity_INCLUDE_DIR
)

if(Unity_FOUND AND NOT TARGET Unity::Unity)
    add_library(Unity::Unity UNKNOWN IMPORTED)
    set_target_properties(Unity::Unity PROPERTIES
        IMPORTED_LOCATION "${Unity_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Unity_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(Unity_INCLUDE_DIR Unity_LIBRARY)