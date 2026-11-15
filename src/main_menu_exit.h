// espgaluda2-recomp (2026-11-15): the main menu's EXIT item quits the program instead of returning to the title
// screen (issue 5), as SDOJ-Recomp's does (github.com/eandis/SDOJ-Recomp, commits e925ff4e "fix game exit" and
// 9691cdfd "exit game", src/exitgame.h: the game's exit path calls ReXApp::RequestExit).

#pragma once

#include <rex/rex_app.h>

namespace espgaluda2::main_menu_exit {

// The app whose RequestExit the EXIT item calls. Set once, before the game runs.
void SetApp(rex::ReXApp& app);

}  // namespace espgaluda2::main_menu_exit
