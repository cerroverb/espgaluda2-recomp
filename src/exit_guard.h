// Original code (WORKLOG.md "Clean exit"): ends the process when the display connection is lost or when the process
// stalls after the game has shut down, instead of letting exit() run global destructors under running threads.

#pragma once

namespace espgaluda2::exit_guard {

// If SDL uses X11 (libX11 already loaded), replaces Xlib's I/O error handler: Xlib's default calls exit() from the UI
// thread while the guest and render threads still run, which crashed (render thread) or hung (pthread_cond_destroy)
// when Xwayland went away. The replacement logs, flushes the log and ends the process with _exit.
void InstallX11IoErrorHandler();

// Starts a detached thread that ends the process with _exit if it is still alive espgaluda2_exit_watchdog_s seconds
// after the game's shutdown began (gamescope's Vulkan WSI layer hung in process exit after its compositor died).
void ArmExitWatchdog();

}  // namespace espgaluda2::exit_guard
