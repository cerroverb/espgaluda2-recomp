// Ported from github.com/ChanseyIsTheBest/futari-nx (pull request 2, commit 7f5a36a), app/src/renut_engine/gpu/guest/widescreen.h. GPL-3.0 (LICENSE-GPL-3.0).
/**
 * @file    gpu/guest/widescreen.h
 * @brief   The game's screen width.
 *
 * futari-nx: Futari's screen is 1280x720, always (N&B's widescreen hooks,
 * guest/widescreen.cpp of banjo-nx's copy, are not Futari's: render/
 * graphics_options.cpp defines this).
 */
#pragma once

#include <cstdint>

namespace renut::gpu {

// The game's screen width (1280; the height is 720).
uint32_t GuestScreenWidth();

} // namespace renut::gpu
