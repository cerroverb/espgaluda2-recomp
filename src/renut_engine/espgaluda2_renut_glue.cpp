// espgaluda2-recomp: cvars Renut's renderer declares and futari-nx defines outside it. Copied from futari-nx
// (github.com/ChanseyIsTheBest/futari-nx, pull request 2, commit 7f5a36a), GPL-3.0 (LICENSE-GPL-3.0), with the
// source path given above each. Categories renamed to this project's.
#include <rex/cvar.h>

// From app/src/me/me_native_system.cpp.
REXCVAR_DEFINE_BOOL(futari_vblank_sleep_exact, true, "Espgaluda2/Renut",
                    "Native renderer: the vblank thread sleeps until the exact next vblank instead of 1 ms steps")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);
REXCVAR_DEFINE_BOOL(futari_vblank_display_sync, true, "Espgaluda2/Renut",
                    "The game's vertical blank follows the display's own refresh (VK_KHR_present_wait: when the "
                    "presents are shown) instead of a 60 Hz timer of its own (input lag, judder); false = the timer")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);
REXCVAR_DEFINE_INT32(futari_vblank_delay_us, 0, "Espgaluda2/Renut",
                     "With futari_vblank_display_sync and futari_vblank_phase_auto off: the game's vertical "
                     "blank comes this many microseconds after the display's refresh (0 = at the refresh)")
    .range(0, 12000)
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);
REXCVAR_DEFINE_BOOL(futari_vblank_phase_auto, true, "Espgaluda2/Renut",
                    "With futari_vblank_display_sync: find the latest guest vblank after the display's refresh "
                    "at which the image is still finished for the next refresh (less input lag); false = "
                    "futari_vblank_delay_us")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);
REXCVAR_DEFINE_INT32(futari_vblank_phase_max_us, 9000, "Espgaluda2/Renut",
                     "futari_vblank_phase_auto: the latest it may put the guest's vblank after the refresh")
    .range(0, 14000)
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

// From app/src/native/futari_native_frame.cpp. futari-nx's own late input (its native frame code) is not ported, so
// this is off here and the renderer's late input (banjo::renut_gpu::LateInputOn, render_thread.cpp) decides.
REXCVAR_DEFINE_BOOL(futari_late_input, false, "Espgaluda2/Renut",
                    "futari-nx's own late input (not in this project; keep false so Renut's applies)")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);
