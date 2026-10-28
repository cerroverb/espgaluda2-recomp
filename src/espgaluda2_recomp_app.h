// espgaluda2_recomp - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/rex_app.h>
#include <rex/system.h>

#include <filesystem>
#include <functional>
#include <optional>

#include "game_setup.h"

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

  // Uses the extracted game in game_data (or a single game folder inside it).
  // If there is none, extracts the .iso beside the game into game_data first.
  std::optional<rex::PathConfig> OnFinalizePaths(
      const rex::PathConfig& defaults,
      std::function<void(rex::PathConfig)> resume) override {
    if (auto folder = game_setup::FindGameFolder(defaults.game_data_root)) {
      auto paths = defaults;
      paths.game_data_root = *folder;
      return paths;
    }
    installer_.Start(
        app_context(), imgui_drawer(), rex::filesystem::GetExecutableFolder(),
        defaults.game_data_root,
        [this, defaults, resume = std::move(resume)](bool ok, const std::string& error) {
          if (ok) {
            if (auto folder = game_setup::FindGameFolder(defaults.game_data_root)) {
              auto paths = defaults;
              paths.game_data_root = *folder;
              resume(paths);
              return;
            }
          }
          rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error,
                                    ok ? "The extracted game files were not found." : error);
          app_context().QuitFromUIThread();
        });
    return std::nullopt;
  }

  void OnShutdown() override { installer_.Stop(); }

 private:
  game_setup::IsoInstaller installer_;
};
