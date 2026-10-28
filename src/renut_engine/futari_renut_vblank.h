// Ported from github.com/ChanseyIsTheBest/futari-nx (pull request 2, commit 7f5a36a), app/src/renut_engine/futari_renut_vblank.h. GPL-3.0 (LICENSE-GPL-3.0).
// futari-nx (2026-10-19): the guest's vertical blank with Renut's renderer (futari_renut_vblank.cpp).
#pragma once

#include <cstdint>

namespace futari::renut_vblank {

// After the runtime is set up (FutariApp::OnPostSetup), with Renut's renderer chosen: the vblank thread (it raises
// vblanks once the title's device exists).
void Start();
void Stop();
// Vblanks raised so far.
uint64_t Count();
// espgaluda2-recomp: waits until Count() >= target (false after timeout_ms, or when the thread stops).
bool WaitForCount(uint64_t target, uint32_t timeout_ms);

}  // namespace futari::renut_vblank
