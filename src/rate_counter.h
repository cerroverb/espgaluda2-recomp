// Original code (WORKLOG.md "Task D"): with espgaluda2_rate_counter on, logs once a second how many game updates
// (the game's Swaps), guest vertical blanks and host presents there were, and the display's refresh rate, to see
// whether the game runs at its own rate or at the host's.

#pragma once

#include <cstdint>
#include <functional>

namespace espgaluda2::rate_counter {

// Starts the thread (it idles while the cvar is off; the cvar can be switched in the F4 settings). `vblanks` and
// `presents` return running totals; an empty function means the renderer does not count it.
void Start(std::function<uint64_t()> vblanks, std::function<uint64_t()> presents);
void Stop();

}  // namespace espgaluda2::rate_counter
