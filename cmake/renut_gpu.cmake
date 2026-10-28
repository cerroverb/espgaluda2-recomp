# Ported from github.com/ChanseyIsTheBest/futari-nx (pull request 2, commit 7f5a36a), app/cmake/futari_renut_gpu.cmake.
# GPL-3.0 (LICENSE-GPL-3.0). Adapted for espgaluda2-recomp: Linux x86-64 only (the Switch, deko3d and game-core parts
# are left out: Espgaluda II's dat*.xex modules link no Direct3D), and plume / XenosRecomp / ImGui come from a fetched
# futari-nx tree instead of a vendored copy.
#
# Renut's Direct3D-level renderer (github.com/masterspike52/reNut by reNut and masterspike52, through banjo-nx's
# adaptation as futari-nx carries it: src/renut_engine/gpu), chosen at boot with --espgaluda2_renderer=renut. No PM4
# ring and no EDRAM model: the game's Direct3D runs natively through plume on Vulkan. Its hooks name the game's
# Direct3D functions (src/renut_engine/espgaluda2_d3d_functions.txt; tools/renut_hooks.py writes the sites).
#
# An OBJECT library, not a static one: its hooks replace the generated code's weak sub_X definitions, and a static
# archive's members are only pulled in for undefined symbols, so the hooks would silently not be linked.

find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")

# futari-nx main at 32a8964 (the same app/thirdparty as PR 2 apart from the dxc-bin change, which
# renut-patches/0001 carries over and extends to x86-64).
include(FetchContent)
file(GLOB _renut_patches "${CMAKE_CURRENT_SOURCE_DIR}/renut-patches/*.patch")
list(SORT _renut_patches)
FetchContent_Declare(futari_nx
    GIT_REPOSITORY https://github.com/ChanseyIsTheBest/futari-nx.git
    GIT_TAG 32a896478b0f6c66f9b2fa7dcc88a2d54e785659
    GIT_PROGRESS TRUE
    UPDATE_DISCONNECTED TRUE
    PATCH_COMMAND ${CMAKE_COMMAND} -DPATCH_DIR=${CMAKE_CURRENT_SOURCE_DIR}/renut-patches -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/apply_patches.cmake
    SOURCE_SUBDIR _unused)
FetchContent_MakeAvailable(futari_nx)
# FetchContent runs its patch step only when it populates the tree; a tree fetched before a patch was added keeps
# missing it. The patch script skips patches already applied, so it runs on every configure too.
execute_process(COMMAND ${CMAKE_COMMAND} -DPATCH_DIR=${CMAKE_CURRENT_SOURCE_DIR}/renut-patches
                        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/apply_patches.cmake
                WORKING_DIRECTORY "${futari_nx_SOURCE_DIR}" RESULT_VARIABLE _patch_result)
if(NOT _patch_result EQUAL 0)
    message(FATAL_ERROR "Applying renut-patches failed")
endif()
set(RENUT_THIRDPARTY_DIR "${futari_nx_SOURCE_DIR}/app/thirdparty")

# The shader containers the shader cache is built from (tools/renut_dump_shaders.py's dump of the loaded default.xex
# image, plus Renut's runtime captures of the shaders the game compiles with D3DX). Game data: in out/, never
# committed. An empty folder is made when there is none, so the first build has no game shaders and the renderer
# captures what it meets (renut_shader_capture_dir); the next build adds the captures.
set(RENUT_SHADER_DUMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/out/renut_shader_dump" CACHE PATH
    "Shader containers for Renut's shader cache (game data)")
if(NOT EXISTS "${RENUT_SHADER_DUMP_DIR}/manifest.csv")
    file(MAKE_DIRECTORY "${RENUT_SHADER_DUMP_DIR}")
    file(WRITE "${RENUT_SHADER_DUMP_DIR}/manifest.csv" "")
endif()
# The renderer's captures (renut_shader_capture_dir, beside the executable) are gpu.cmake's default
# RENUT_SHADER_CAPTURE_DIR, ${CMAKE_BINARY_DIR}/shader_capture: the executable is built there.
set(RENUT_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/out/unused" CACHE PATH "Unused (the dump folder always exists)")

set(BANJO_RENUT_XENOS_RECOMP_ARGS "")
if(FUTARI_DXC AND NOT DXC_SYSTEM_TOOL)
    set(DXC_SYSTEM_TOOL "${FUTARI_DXC}")
endif()
foreach(_renut_var CMAKE_C_COMPILER_TARGET CMAKE_CXX_COMPILER_TARGET CMAKE_LINKER_TYPE CMAKE_C_FLAGS CMAKE_CXX_FLAGS
                   CMAKE_AR CMAKE_MAKE_PROGRAM DXC_SYSTEM_TOOL DXC_SYSTEM_LIB)
    if(DEFINED ${_renut_var} AND NOT "${${_renut_var}}" STREQUAL "")
        list(APPEND BANJO_RENUT_XENOS_RECOMP_ARGS "-D${_renut_var}=${${_renut_var}}")
    endif()
endforeach()

if(TARGET o1heap AND NOT TARGET rex::o1heap)
    add_library(rex::o1heap ALIAS o1heap)
endif()

# The SDK's window is SDL3 (sdk-patches/0006): plume makes its surface through SDL (renut-patches/0003), so the
# renderer presents on Wayland or X11, whichever SDL picked (the SDK's video_driver cvar can force one).
set(PLUME_SDL_VULKAN_ENABLED ON CACHE BOOL "" FORCE)
set(PLUME_SDL3_TARGET SDL3::SDL3-static CACHE STRING "" FORCE)
include("${CMAKE_CURRENT_SOURCE_DIR}/src/renut_engine/gpu/gpu.cmake")
# plume's copy of volk under its own names (renut-patches/0002): ReXGlue 0.8.0 links its volk statically into the same
# executable. PUBLIC, so the renderer's own Vulkan calls name plume's pointers too.
target_compile_definitions(plume PUBLIC PLUME_RENAME_VOLK)

add_library(espgaluda2_renut_gpu OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/src/renut_engine/gpu/banjo_renut_select.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/renut_engine/gpu/banjo_renut_tuning.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/renut_engine/gpu/banjo_renut_checks.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/renut_engine/gpu/banjo_renut_record.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/renut_engine/display_sync.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/renut_engine/perf/frame_stats.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/renut_engine/futari_renut_shim.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/renut_engine/futari_renut_vblank.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/renut_engine/espgaluda2_renut_glue.cpp")
target_include_directories(espgaluda2_renut_gpu PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${REXGLUE_ENTRYPOINT_INCLUDE_DIR}")
target_compile_definitions(espgaluda2_renut_gpu PRIVATE FUTARI_RENUT_RENDERER=1)
target_link_libraries(espgaluda2_renut_gpu PRIVATE rex::runtime)
renut_gpu_setup(espgaluda2_renut_gpu)
target_link_libraries(espgaluda2_recomp PRIVATE espgaluda2_renut_gpu)
# FUTARI_RENUT_RENDERER for the app sources that use it: set in CMakeLists.txt per source.
