// Original code (WORKLOG.md "Clean exit"); see exit_guard.h.

#include "exit_guard.h"

#include <chrono>
#include <cstdlib>
#include <thread>

#include <cstring>

#include <SDL3/SDL_video.h>

#include <rex/cvar.h>
#include <rex/logging.h>

#if defined(__linux__)
#include <dlfcn.h>
#include <unistd.h>
#endif

REXCVAR_DEFINE_INT32(espgaluda2_exit_watchdog_s, 5, "Espgaluda2",
                     "Seconds after the shutdown began before a stalled process is ended (0: never)");

namespace espgaluda2::exit_guard {

namespace {

[[noreturn]] void EndProcess(int code) {
  rex::ShutdownLogging();
#if defined(__linux__)
  _exit(code);
#else
  std::_Exit(code);
#endif
}

#if defined(__linux__)
// Xlib's XIOErrorHandler: int (*)(Display*). Xlib exits itself if the handler returns.
int X11IoError(void*) {
  REXLOG_ERROR("X11 display connection lost: ending the process");
  EndProcess(EXIT_FAILURE);
}
#endif

}  // namespace

void InstallX11IoErrorHandler() {
#if defined(__linux__)
  // Only with SDL's x11 video driver, whose libX11 is already loaded (never loads it).
  const char* driver = SDL_GetCurrentVideoDriver();
  if (!driver || std::strcmp(driver, "x11") != 0) {
    return;
  }
  void* x11 = dlopen("libX11.so.6", RTLD_NOW | RTLD_NOLOAD);
  if (!x11) {
    return;
  }
  using Handler = int (*)(void*);
  using SetHandler = Handler (*)(Handler);
  if (auto set = reinterpret_cast<SetHandler>(dlsym(x11, "XSetIOErrorHandler"))) {
    set(X11IoError);
    REXLOG_INFO("X11: I/O error handler installed (a lost display ends the process)");
  }
  dlclose(x11);
#endif
}

void ArmExitWatchdog() {
  const int seconds = REXCVAR_GET(espgaluda2_exit_watchdog_s);
  if (seconds <= 0) {
    return;
  }
  std::thread([seconds] {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    REXLOG_ERROR("exit stalled {} s after the shutdown began: ending the process", seconds);
    EndProcess(EXIT_SUCCESS);
  }).detach();
}

}  // namespace espgaluda2::exit_guard
