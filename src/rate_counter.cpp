// Original code (WORKLOG.md "Task D"); see rate_counter.h.

#include "rate_counter.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include <SDL3/SDL_video.h>

#include <rex/cvar.h>
#include <rex/frame_latency.h>
#include <rex/logging.h>

REXCVAR_DEFINE_BOOL(espgaluda2_rate_counter, false, "Espgaluda2",
                    "Log once a second: game updates (Swaps), guest vblanks and host presents a second, and the "
                    "display's refresh rate (lines starting [rate])");

namespace espgaluda2::rate_counter {

namespace {

std::thread g_thread;
std::mutex g_mutex;
std::condition_variable g_wake;
bool g_stop = false;

// The refresh rate of the display the game window is on (SDL); 0 when unknown.
float DisplayHz() {
  int count = 0;
  SDL_Window** windows = SDL_GetWindows(&count);
  SDL_Window* window = (windows && count > 0) ? windows[0] : nullptr;
  SDL_free(windows);
  const SDL_DisplayID display = window ? SDL_GetDisplayForWindow(window) : 0;
  const SDL_DisplayMode* mode = display ? SDL_GetCurrentDisplayMode(display) : nullptr;
  return mode ? mode->refresh_rate : 0.0f;
}

std::string PerSecond(uint64_t delta, double seconds) {
  return std::to_string(int64_t(double(delta) / seconds + 0.5));
}

void Run(std::function<uint64_t()> vblanks, std::function<uint64_t()> presents) {
  using clock = std::chrono::steady_clock;
  bool was_on = false;
  uint64_t swaps0 = 0, vblanks0 = 0, presents0 = 0;
  clock::time_point t0;
  std::unique_lock lock(g_mutex);
  while (!g_stop) {
    g_wake.wait_for(lock, std::chrono::seconds(1), [] { return g_stop; });
    if (g_stop) break;
    const bool on = REXCVAR_GET(espgaluda2_rate_counter);
    const clock::time_point now = clock::now();
    const uint64_t swaps1 = rex::latency::SwapCount();
    const uint64_t vblanks1 = vblanks ? vblanks() : 0;
    const uint64_t presents1 = presents ? presents() : 0;
    if (on && was_on) {
      const double s = std::chrono::duration<double>(now - t0).count();
      REXLOG_INFO("[rate] {:.2f} s: {} game updates/s, {} guest vblanks/s, {} host presents/s; display {:.2f} Hz",
                  s, PerSecond(swaps1 - swaps0, s), vblanks ? PerSecond(vblanks1 - vblanks0, s) : "(not counted)",
                  presents ? PerSecond(presents1 - presents0, s) : "(not counted)", DisplayHz());
    }
    was_on = on;
    swaps0 = swaps1;
    vblanks0 = vblanks1;
    presents0 = presents1;
    t0 = now;
  }
}

}  // namespace

void Start(std::function<uint64_t()> vblanks, std::function<uint64_t()> presents) {
  if (g_thread.joinable()) return;
  g_stop = false;
  g_thread = std::thread(Run, std::move(vblanks), std::move(presents));
}

void Stop() {
  {
    std::lock_guard lock(g_mutex);
    g_stop = true;
  }
  g_wake.notify_all();
  if (g_thread.joinable()) g_thread.join();
}

}  // namespace espgaluda2::rate_counter
