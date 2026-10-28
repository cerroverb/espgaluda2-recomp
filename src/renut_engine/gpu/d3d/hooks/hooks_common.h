// Ported from github.com/ChanseyIsTheBest/futari-nx (pull request 2, commit 7f5a36a), app/src/renut_engine/gpu/d3d/hooks/hooks_common.h. GPL-3.0 (LICENSE-GPL-3.0).
/**
 * @file    gpu/d3d/hooks/hooks_common.h
 * @brief   No-op hook macros for guest D3D calls the host renderer does not model.
 *
 * Same shape as reblue's BD_NOOP / BD_NOOP_RETURN: REX_STUB logs on every
 * call, so it is for unimplemented placeholders; these are silent.
 */
#pragma once

#include <rex/hook.h>

// Intentional no-op with no log output.
#define RENUT_NOOP(subroutine)                                                 \
  extern "C" REX_FUNC(subroutine) {                                            \
    (void)ctx;                                                                 \
    (void)base;                                                                \
  }

// RENUT_NOOP that returns a constant u32 in r3, for callers checking the result.
#define RENUT_NOOP_RETURN(subroutine, value)                                   \
  extern "C" REX_FUNC(subroutine) {                                            \
    (void)base;                                                                \
    ctx.r3.u64 = (value);                                                      \
  }
