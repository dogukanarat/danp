#[[
    GitVersion.cmake - Git-based version identification module
    
    Usage:
        include(GitVersion)
        get_git_version(VAR_SEMVER VAR_FULL)
        
        project(MyLib VERSION ${VAR_SEMVER})
        # Use ${VAR_FULL} for config.h or logging
]]

function(get_git_version OUT_SEMVER OUT_FULL_STRING)
    set(_full_version "unknown")
    set(_sem_version "0.0.1") # Default fallback if no git/tags found

    # 1. Find Git
    find_package(Git QUIET)
    if(NOT GIT_FOUND)
        set(${OUT_SEMVER} "${_sem_version}" PARENT_SCOPE)
        set(${OUT_FULL_STRING} "${_full_version}" PARENT_SCOPE)
        return()
    endif()

    # 2. Check if inside a Git Repo
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --is-inside-work-tree
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE _git_repo_check
        OUTPUT_QUIET ERROR_QUIET
    )
    if(NOT 0 EQUAL _git_repo_check)
        set(${OUT_SEMVER} "${_sem_version}" PARENT_SCOPE)
        set(${OUT_FULL_STRING} "${_full_version}" PARENT_SCOPE)
        return()
    endif()

    # 3. Get Tag (Exact Match)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --exact-match
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE _tag_result
        OUTPUT_VARIABLE _tag
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
    )

    # 4. Get Short Hash
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE _hash
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
    )

    # 5. Determine Base Version String
    if(0 EQUAL _tag_result)
        set(_full_version "${_tag}")
    else()
        set(_full_version "${_hash}")
    endif()

    # 6. Check Dirty State
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE _dirty_result
        OUTPUT_QUIET ERROR_QUIET
    )

    if(NOT 0 EQUAL _dirty_result)
        # Get username for dirty string
        execute_process(
            COMMAND ${GIT_EXECUTABLE} config user.name
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE _username
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
        )
        if("${_username}" STREQUAL "")
            set(_username "anonymous")
        endif()
        string(REPLACE " " "_" _username "${_username}")
        set(_full_version "${_full_version}-dirty-${_username}")
    endif()

    # --------------------------------------------------------------------------
    # Logic to extract Semantic Version (X.Y.Z) for CMake
    # --------------------------------------------------------------------------
    
    # Remove leading 'v' (v1.2.3 -> 1.2.3)
    string(REGEX REPLACE "^v" "" _clean_ver "${_full_version}")

    # Extract X.Y.Z (Major.Minor.Patch)
    string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" _extracted_semver "${_clean_ver}")

    if(NOT "${_extracted_semver}" STREQUAL "")
        set(_sem_version "${_extracted_semver}")
    else()
        # Warn if we are on a commit hash (e.g. a1b2c3d) without a semver tag
        # message(STATUS "Git version '${_full_version}' is not a valid SemVer. Defaulting to ${_sem_version}")
    endif()

    # Return values
    set(${OUT_SEMVER} "${_sem_version}" PARENT_SCOPE)
    set(${OUT_FULL_STRING} "${_full_version}" PARENT_SCOPE)
endfunction()