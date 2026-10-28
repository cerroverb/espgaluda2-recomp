# espgaluda2_recomp - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(espgaluda2_recomp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(REXGLUE_ENABLE_TRACY "Enable Tracy profiler integration" OFF)
option(REXGLUE_ENABLE_PERF_COUNTERS "Enable lightweight perf counter registry and overlay" OFF)

# ReXGlue SDK: the copy bundled in SDOJ-Recomp at a pinned commit, with the
# patches in sdk-patches applied. Pass -DREXSDK_DIR to use another tree.
set(REXSDK_DIR "" CACHE PATH "Path to rexglue-sdk source tree")
if(NOT REXSDK_DIR)
    include(FetchContent)
    file(GLOB _sdk_patches "${CMAKE_CURRENT_SOURCE_DIR}/sdk-patches/*.patch")
    list(SORT _sdk_patches)
    FetchContent_Declare(sdoj_recomp
        GIT_REPOSITORY https://github.com/eandis/SDOJ-Recomp.git
        GIT_TAG ea1514977c45356038b40631c087021c2991fa47
        GIT_PROGRESS TRUE
        UPDATE_DISCONNECTED TRUE
        PATCH_COMMAND git apply --whitespace=nowarn ${_sdk_patches}
        # Only the SDK inside is used; don't add SDOJ's own project.
        SOURCE_SUBDIR _unused
    )
    FetchContent_MakeAvailable(sdoj_recomp)
    set(REXSDK_DIR "${sdoj_recomp_SOURCE_DIR}/thirdparty/rexglue-sdk" CACHE PATH
        "Path to rexglue-sdk source tree" FORCE)
endif()

include(generated/rexglue.cmake)

# extract-xiso, run on first launch to extract an ISO into game_data.
include(FetchContent)
FetchContent_Declare(extract_xiso
    GIT_REPOSITORY https://github.com/XboxDev/extract-xiso.git
    GIT_TAG 3f5b62cfe68f000b0e3c8a30104973f3a297948e
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(extract_xiso)

# Sources
set(ESPGALUDA2RECOMP_SOURCES
    src/main.cpp
    src/game_setup.cpp
    src/latency_patch.cpp
)

if(WIN32)
    add_executable(espgaluda2_recomp WIN32 ${ESPGALUDA2RECOMP_SOURCES})
else()
    add_executable(espgaluda2_recomp ${ESPGALUDA2RECOMP_SOURCES})
endif()

rexglue_setup_target(espgaluda2_recomp)

if(NOT WIN32)
    # copy the runtime libraries next to the executable
    set(_runtime_libs $<TARGET_FILE:rexruntime>)
    if(TARGET TracyClient)
        list(APPEND _runtime_libs $<TARGET_FILE:TracyClient>)
    endif()
    add_custom_command(TARGET espgaluda2_recomp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${_runtime_libs}
            "$<TARGET_FILE_DIR:espgaluda2_recomp>"
        VERBATIM
    )
endif()

# Stage extract-xiso and its license in tools/extract-xiso beside the executable.
set(_extract_xiso_dir "$<TARGET_FILE_DIR:espgaluda2_recomp>/tools/extract-xiso")
add_custom_target(stage_extract_xiso ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_extract_xiso_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:extract-xiso>" "${_extract_xiso_dir}/$<TARGET_FILE_NAME:extract-xiso>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${extract_xiso_SOURCE_DIR}/LICENSE.TXT" "${_extract_xiso_dir}/LICENSE.TXT"
    VERBATIM
)
add_dependencies(stage_extract_xiso extract-xiso espgaluda2_recomp)
