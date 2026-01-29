# ==============================================================================
# Dependency: Unity Test Framework
# ==============================================================================

# 1. Try to find Unity on the system
find_package(unity CONFIG QUIET)
find_package(PkgConfig QUIET)

# 2. Identify the correct target name (System vs PkgConfig)
if(TARGET unity::framework)          # Your specific system target
    set(UNITY_TARGET unity::framework)
elseif(TARGET unity::unity)          # Common alternative
    set(UNITY_TARGET unity::unity)
elseif(TARGET unity)                 # Common alternative
    set(UNITY_TARGET unity)
elseif(PKG_CONFIG_FOUND)             # Pkg-Config fallback
    pkg_check_modules(UnityPkg QUIET IMPORTED_TARGET unity)
    if(UnityPkg_FOUND)
        set(UNITY_TARGET PkgConfig::UnityPkg)
    endif()
endif()

# 3. Fallback: Download from GitHub if no system target was found
if(NOT UNITY_TARGET)
    message(STATUS "Unity not found on system. Fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        Unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG        v2.6.0
    )
    FetchContent_MakeAvailable(Unity)
    set(UNITY_TARGET unity)
endif()

if(NOT TARGET Unity::unity)
    add_library(Unity::unity ALIAS ${UNITY_TARGET})
endif()

# ==============================================================================
# Test Executable
# ==============================================================================

set(TEST_NAME ${PROJECT_NAME}UnitTest)

add_executable(${TEST_NAME} 
    ${CMAKE_CURRENT_SOURCE_DIR}/test_danp.c
)

target_link_libraries(${TEST_NAME}
    PRIVATE
        ${PROJECT_NAME}::${PROJECT_NAME}
        Unity::unity 
)

add_test(
    NAME ${TEST_NAME}
    COMMAND ${TEST_NAME}
)