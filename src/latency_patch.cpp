// espgaluda2_recomp - input latency patch
//
// The game's D3D Swap (sub_82170CE0) saves the fence of the frame it just
// submitted in device+15148 and only waits for it at the *next* Swap (inside
// sub_82173DC8). The CPU therefore runs a full frame ahead of a GPU that flips
// on vblank, so a pad read reaches the screen about two frames later.
//
// With the patch, Swap waits for its own fence before returning, using the
// game's own fence wait (sub_82173AE8). The next frame's game logic and pad
// read then start one frame later, closer to the flip, at the same 60 Hz pace.
// Measured in the menu: pad read -> host present ~32 ms without the patch.
//
// Disable with --latency_patch=false.

#include "generated/default/espgaluda2_recomp_init.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(latency_patch, true, "Espgaluda2",
                    "Wait for each frame's GPU fence in Swap (cuts about one frame of input lag)");

namespace {
// Device fields used by the game's D3D (guest offsets, see WORKLOG.md).
constexpr uint32_t kThrottleFenceOffset = 15148;  // fence saved by Swap
constexpr uint32_t kDeviceFlagsOffset = 24192;    // bit 0: throttling off
constexpr uint32_t kFenceWaitMode = 3;            // r5 used by the game's own calls
}  // namespace

extern "C" REX_FUNC(__imp__sub_82170CE0);
extern "C" REX_FUNC(sub_82173AE8);

extern "C" REX_FUNC(sub_82170CE0) {
  const uint32_t device = ctx.r3.u32;
  __imp__sub_82170CE0(ctx, base);

  if (!REXCVAR_GET(latency_patch) || device == 0) {
    return;
  }
  const uint32_t fence = REX_LOAD_U32(device + kThrottleFenceOffset);
  if (fence == 0 || (REX_LOAD_U32(device + kDeviceFlagsOffset) & 1) != 0) {
    return;
  }

  // Keep Swap's return value and link register; the wait is a normal guest call
  // that preserves the non-volatile registers.
  const PPCRegister saved_r3 = ctx.r3;
  const uint64_t saved_lr = ctx.lr;
  ctx.r3.u64 = device;
  ctx.r4.u64 = fence;
  ctx.r5.u64 = kFenceWaitMode;
  ctx.r6.u64 = 0;
  ctx.r7.u64 = 0;
  sub_82173AE8(ctx, base);
  ctx.r3 = saved_r3;
  ctx.lr = saved_lr;
}
