# Applies each *.patch of PATCH_DIR, in name order, with git apply in the current directory, skipping a patch that is already
# applied (its reverse applies). FetchContent re-runs its patch step whenever the configure step changes, on a tree
# that is already patched; plain `git apply` then fails. Original code (WORKLOG.md "Original code").
#   cmake -DPATCH_DIR=<dir> -P apply_patches.cmake
file(GLOB PATCHES "${PATCH_DIR}/*.patch")
list(SORT PATCHES)
foreach(_patch IN LISTS PATCHES)
    execute_process(COMMAND git apply --reverse --check --whitespace=nowarn "${_patch}"
                    RESULT_VARIABLE _applied OUTPUT_QUIET ERROR_QUIET)
    if(_applied EQUAL 0)
        message(STATUS "already applied: ${_patch}")
        continue()
    endif()
    execute_process(COMMAND git apply --check --whitespace=nowarn "${_patch}"
                    RESULT_VARIABLE _applies OUTPUT_QUIET ERROR_QUIET)
    if(NOT _applies EQUAL 0)
        # Applies neither way. Only one patch may: sdk-patches/0001, applied earlier and overlapped by 0005 (both
        # change graphics_system.cpp), so its reverse no longer matches. Anything else is a real failure (a lenient
        # rule here once let a broken 0006 through silently).
        get_filename_component(_name "${_patch}" NAME)
        if(_name STREQUAL "0001-display-timing-present-wait.patch")
            message(STATUS "applied earlier (0005 overlaps it): ${_patch}")
            continue()
        endif()
        message(FATAL_ERROR "${_patch} applies neither forward nor in reverse: the tree is in an unknown state "
                            "(delete the fetched source under _deps to fetch it again)")
    endif()
    execute_process(COMMAND git apply --whitespace=nowarn "${_patch}" RESULT_VARIABLE _result)
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "git apply failed: ${_patch}")
    endif()
    message(STATUS "applied: ${_patch}")
endforeach()
