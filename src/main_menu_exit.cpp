// espgaluda2-recomp (2026-11-15); see main_menu_exit.h. The hook is original code; the RequestExit call is
// SDOJ-Recomp's (src/exitgame.h, pc_exit::quit).
//
// The main menu's update, sub_820C9748 (generated/default), keeps its state in bytes of its object (r3): 2372 the
// scene, 2373 the menu's step, 2374 the step's phase, and the cursor in the word at 2376 (items 0 to 6). Choosing item
// 6, EXIT, sets the step to 6. Step 6 waits on sub_820BAF38, shows XBOX_SYS_TEXT_60 ("Returning to the Title
// Screen... Proceed?") when sub_82091B08 or sub_82091B18 returns 1, fades out (phases 100 and 101) and in phase 102
// sets the scene to 2 and the step to 0. NO sets the step to 1 (the menu). Step 6 to step 0 is therefore the
// confirmed exit, after the fade, and only that.
#include "main_menu_exit.h"

#include <rex/cvar.h>
#include <rex/logging.h>

#include "generated/default/espgaluda2_recomp_init.h"

REXCVAR_DEFINE_BOOL(exit_to_desktop, true, "Espgaluda2",
                    "The main menu's EXIT item quits the program (off: it returns to the title screen, as on the "
                    "Xbox 360)");

extern "C" REX_FUNC(__imp__sub_820C9748);

namespace espgaluda2::main_menu_exit {
namespace {

constexpr uint32_t kStepOffset = 2373;
constexpr uint8_t kExitStep = 6;
constexpr uint8_t kDoneStep = 0;

rex::ReXApp* g_app = nullptr;

}  // namespace

void SetApp(rex::ReXApp& app) { g_app = &app; }

}  // namespace espgaluda2::main_menu_exit

extern "C" REX_FUNC(sub_820C9748) {
  using namespace espgaluda2::main_menu_exit;
  const uint32_t step_address = ctx.r3.u32 + kStepOffset;
  const bool exiting = REX_LOAD_U8(step_address) == kExitStep;
  __imp__sub_820C9748(ctx, base);
  if (exiting && REX_LOAD_U8(step_address) == kDoneStep && REXCVAR_GET(exit_to_desktop) && g_app) {
    REXLOG_INFO("main menu EXIT: quitting (exit_to_desktop)");
    g_app->RequestExit();
  }
}
