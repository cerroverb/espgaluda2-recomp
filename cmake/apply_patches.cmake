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
    execute_process(COMMAND git apply --whitespace=nowarn "${_patch}" RESULT_VARIABLE _result)
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "git apply failed: ${_patch}")
    endif()
    message(STATUS "applied: ${_patch}")
endforeach()
