# Applies each *.patch of PATCH_DIR, in name order, with git apply in the current directory. FetchContent re-runs its
# patch step whenever the configure step changes, on a tree that is already patched; plain `git apply` then fails.
# Original code (WORKLOG.md "Original code").
#   cmake -DPATCH_DIR=<dir> -P apply_patches.cmake
# A copy of the applied set is kept in .applied-patches/ of the patched tree. The same set again: nothing to do. A
# changed set: the kept copies are reversed (last first) and the new set applied, so a patch that a later one changes
# (0006 creates windowed_app_context_sdl.cpp, 0007 edits it) can itself change. Without a kept copy (a tree patched
# before it existed), each patch is skipped when its reverse applies.
file(GLOB PATCHES "${PATCH_DIR}/*.patch")
list(SORT PATCHES)
set(_kept_dir "${CMAKE_CURRENT_SOURCE_DIR}/.applied-patches")

function(_git_apply _patch)
    execute_process(COMMAND git apply --whitespace=nowarn ${ARGN} "${_patch}" RESULT_VARIABLE _result)
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "git apply ${ARGN} failed: ${_patch}")
    endif()
endfunction()

if(EXISTS "${_kept_dir}")
    file(GLOB _kept "${_kept_dir}/*.patch")
    list(SORT _kept)
    set(_same TRUE)
    list(LENGTH _kept _kept_count)
    list(LENGTH PATCHES _count)
    if(NOT _kept_count EQUAL _count)
        set(_same FALSE)
    else()
        foreach(_patch IN LISTS PATCHES)
            get_filename_component(_name "${_patch}" NAME)
            file(SHA256 "${_patch}" _new)
            if(EXISTS "${_kept_dir}/${_name}")
                file(SHA256 "${_kept_dir}/${_name}" _old)
            else()
                set(_old "")
            endif()
            if(NOT _new STREQUAL _old)
                set(_same FALSE)
            endif()
        endforeach()
    endif()
    if(_same)
        message(STATUS "patches already applied: ${PATCH_DIR}")
        return()
    endif()
    list(REVERSE _kept)
    foreach(_patch IN LISTS _kept)
        _git_apply("${_patch}" --reverse)
        message(STATUS "reversed: ${_patch}")
    endforeach()
    file(REMOVE_RECURSE "${_kept_dir}")
    foreach(_patch IN LISTS PATCHES)
        _git_apply("${_patch}")
        message(STATUS "applied: ${_patch}")
    endforeach()
else()
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
            # change graphics_system.cpp), so its reverse no longer matches. Anything else is a real failure (a
            # lenient rule here once let a broken 0006 through silently).
            get_filename_component(_name "${_patch}" NAME)
            if(_name STREQUAL "0001-display-timing-present-wait.patch")
                message(STATUS "applied earlier (0005 overlaps it): ${_patch}")
                continue()
            endif()
            message(FATAL_ERROR "${_patch} applies neither forward nor in reverse: the tree is in an unknown state "
                                "(delete the fetched source under _deps to fetch it again)")
        endif()
        _git_apply("${_patch}")
        message(STATUS "applied: ${_patch}")
    endforeach()
endif()
file(MAKE_DIRECTORY "${_kept_dir}")
file(COPY ${PATCHES} DESTINATION "${_kept_dir}")
