// espgaluda2_recomp - ReXGlue Recompiled Project

#include "generated/default/espgaluda2_recomp_init.h"

#include "espgaluda2_recomp_app.h"

#if defined(__linux__)
#include <cstdlib>

// espgaluda2-recomp (2026-10-29, as futari-linux a1f2241): runs before main, so before the SDK initialises SDL. The app
// id is what the taskbar groups the window by and names it after (the Wayland app_id, the X11 WM_CLASS), as
// UnleashedRecomp sets its own before creating its window (github.com/hedge-dev/UnleashedRecomp cf829a9,
// ui/game_window.cpp, SDL_SetHint("SDL_APP_ID", ...)); SDL reads hints from the environment as well. An explicit user
// setting is left alone.
[[maybe_unused]] static const bool kAppId = [] {
#if defined(__aarch64__)
  setenv("SDL_APP_ID", "espgaluda2-recomp-aarch64", 0);
#else
  setenv("SDL_APP_ID", "espgaluda2-recomp-x86_64", 0);
#endif
  return true;
}();
#endif

REX_DEFINE_APP(espgaluda2_recomp, Espgaluda2RecompApp::Create)
