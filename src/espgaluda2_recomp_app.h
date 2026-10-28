// espgaluda2_recomp - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/rex_app.h>

#include <filesystem>

class Espgaluda2RecompApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Espgaluda2RecompApp>(new Espgaluda2RecompApp(ctx, "espgaluda2_recomp",
        PPCImageConfig));
  }

  // Game files are read from a game_data folder beside the executable unless
  // --game_data_root is passed. User data (saves, config, cache) goes in
  // user_data beside the executable unless --user_data_root is passed.
  void OnConfigurePaths(rex::PathConfig& paths) override {
    const auto exe_dir = rex::filesystem::GetExecutableFolder();
    if (REXCVAR_GET(game_data_root).empty()) {
      paths.game_data_root = exe_dir / "game_data";
    }
    if (REXCVAR_GET(user_data_root).empty()) {
      paths.user_data_root = exe_dir / "user_data";
      if (REXCVAR_GET(cache_path).empty()) {
        paths.cache_root = paths.user_data_root / "cache";
      }
    }
  }
};
