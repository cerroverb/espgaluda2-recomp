// espgaluda2-recomp: debug aid (original code, WORKLOG.md "Original code"). With --espgaluda2_debug_xmem=true, the
// game's XMemAlloc / XMemFree (confirmed in docs/d3d/map_B.txt: XMemFree sub_8215F3F8 sends attribute bit 31 to
// XPhysicalFree and the rest to the heap free sub_8215F8D8; XMemAlloc sub_8215F360 takes (size r3, attributes r4)
// and returns the block in r3) keep a set of live blocks, and every free of a block that is not live (a double free,
// or a pointer the game's allocator never returned) is logged with its caller. Off: one byte load per call.

#include "generated/default/espgaluda2_recomp_init.h"

#include <mutex>
#include <unordered_map>

#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DEFINE_BOOL(espgaluda2_debug_xmem, false, "Espgaluda2/Debug",
                    "Log XMemFree calls on blocks XMemAlloc did not return (or freed twice), with their callers")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

extern "C" REX_FUNC(__imp__sub_8215F360);
extern "C" REX_FUNC(__imp__sub_8215F3F8);

namespace {
// The game heap's free list of large blocks, walked by the heap allocator sub_820E24B0 (circular, head at heap+384,
// next pointer at +0 of each node). The heap object is *(0x82947DA4) (sub_820E3520: lis 0x8294; lwz 32164).
constexpr uint32_t kHeapPointerVa = 0x82947DA4u;
constexpr uint32_t kFreeListOffset = 384;
uint8_t* g_base = nullptr;
bool g_broken = false;
const char* g_last_ok = "start";

uint32_t Load32(uint32_t va) {
  const uint8_t* p = g_base + va;
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}

// true when the list is a proper cycle through its head (or the heap does not exist yet).
bool FreeListOk() {
  if (!g_base) return true;
  const uint32_t heap = Load32(kHeapPointerVa);
  if (!heap) return true;
  const uint32_t head = heap + kFreeListOffset;
  uint32_t node = Load32(head);
  for (int i = 0; i < 1000000; ++i) {
    if (node == head) return true;
    if (node < 0x40000000u) return false;  // not a heap address
    node = Load32(node);
  }
  return false;
}
std::mutex g_mutex;
std::unordered_map<uint32_t, uint32_t> g_live;  // block -> attributes
uint64_t g_bad = 0;
}  // namespace

// Called after every Renut hook entry and around XMemAlloc / XMemFree when espgaluda2_debug_xmem is on: logs the
// first point at which the free list is no longer a cycle, and what ran last while it still was.
extern "C" void Espgaluda2DebugHeapCheck(const char* where, uint8_t* base) {
  static const bool on = REXCVAR_GET(espgaluda2_debug_xmem);
  if (!on || g_broken) return;
  g_base = base;
  if (FreeListOk()) {
    g_last_ok = where;
    return;
  }
  g_broken = true;
  REXLOG_ERROR("[xmem] game heap free list broken after {} (last check that passed: {})", where, g_last_ok);
}

extern "C" REX_FUNC(sub_8215F360) {
  static const bool on = REXCVAR_GET(espgaluda2_debug_xmem);
  const uint32_t size = ctx.r3.u32, attributes = ctx.r4.u32, caller = uint32_t(ctx.lr);
  Espgaluda2DebugHeapCheck("before XMemAlloc", base);
  __imp__sub_8215F360(ctx, base);
  Espgaluda2DebugHeapCheck("XMemAlloc", base);
  if (!on || !ctx.r3.u32) return;
  std::lock_guard lock(g_mutex);
  if (g_live.count(ctx.r3.u32))
    REXLOG_WARN("[xmem] XMemAlloc({:#x}, {:08X}) from {:08X} returned live block {:08X}", size, attributes, caller,
                ctx.r3.u32);
  g_live[ctx.r3.u32] = attributes;
}

extern "C" REX_FUNC(sub_8215F3F8) {
  static const bool on = REXCVAR_GET(espgaluda2_debug_xmem);
  if (on && ctx.r3.u32) {
    const uint32_t va = ctx.r3.u32, attributes = ctx.r4.u32, caller = uint32_t(ctx.lr);
    std::lock_guard lock(g_mutex);
    auto it = g_live.find(va);
    if (it == g_live.end()) {
      if (++g_bad <= 200)
        REXLOG_WARN("[xmem] XMemFree({:08X}, {:08X}) from {:08X}: not a live XMemAlloc block", va, attributes,
                    caller);
    } else {
      if ((it->second ^ attributes) & 0x80000000u)
        REXLOG_WARN("[xmem] XMemFree({:08X}, {:08X}) from {:08X}: allocated with {:08X} (other allocator)", va,
                    attributes, caller, it->second);
      g_live.erase(it);
    }
  }
  Espgaluda2DebugHeapCheck("before XMemFree", base);
  __imp__sub_8215F3F8(ctx, base);
  Espgaluda2DebugHeapCheck("XMemFree", base);
}
