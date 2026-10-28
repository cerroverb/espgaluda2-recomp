// espgaluda2_recomp - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/cvar.h>
#include <rex/graphics/graphics_system.h>
#include <rex/filesystem.h>
#include <rex/rex_app.h>
#include <rex/system.h>
#include <rex/ui/keybinds.h>
#include <rex/ui/window.h>

#include <filesystem>
#include <functional>
#include <optional>

#include "exit_guard.h"
#include "futari_capture.h"
#include "futari_input_script.h"
#include "game_setup.h"

#if FUTARI_RENUT_RENDERER
// Renut's Direct3D-level renderer (--espgaluda2_renderer=renut; cmake/renut_gpu.cmake). The hooks below are
// futari-nx's (github.com/ChanseyIsTheBest/futari-nx, pull request 2, commit 7f5a36a, app/src/futari_app.h,
// GPL-3.0), unchanged in what they do.
#include <rex/frame_latency.h>
#include <rex/runtime.h>
#include <rex/ui/immediate_drawer.h>

#include "renut_engine/futari_renut_vblank.h"
#include "renut_engine/gpu/banjo_renut_capture.h"
#include "renut_engine/gpu/banjo_renut_select.h"
#include "renut_engine/gpu/overlay/app_integration.h"
#endif

#include <cstdio>
#include <cstdlib>

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
#if FUTARI_RENUT_RENDERER
    // futari-nx futari_app.h: the window exists here (UI thread, before the game starts): Renut's renderer comes up
    // first.
    if (banjo::renut_gpu::Active()) {
      renut::gpu::StartRenderer(app_context(), [this] { return imgui_drawer(); });
    }
#endif
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

  // futari-nx futari_app.h OnPreSetup: with Renut's renderer, no SDK graphics system and no GPU plugin (the SDK's
  // detached mode, sdk-patches/0002).
  void OnPreSetup(rex::RuntimeConfig& config) override {
#if FUTARI_RENUT_RENDERER
    if (banjo::renut_gpu::Active()) {
      renut::gpu::ConfigureRuntime(config);
      REXLOG_INFO("graphics: Renut's Direct3D-level renderer (plume; no PM4 ring, no EDRAM model)");
      return;
    }
#endif
    (void)config;
    REXLOG_INFO("graphics: the SDK's emulated Xenos GPU");
  }

  // futari-nx futari_app.h OnPostSetup: the scripted pad, the recorder and Renut's vblank thread.
  void OnPostSetup() override {
    espgaluda2::exit_guard::InstallX11IoErrorHandler();  // original code, src/exit_guard.h
    // UnleashedRecomp ui/game_window.cpp (github.com/hedge-dev/UnleashedRecomp cf829a9) toggles fullscreen on Alt+Enter
    // and stores it in its Fullscreen config; the same here through the SDK's keybinds, which take single keys: F11,
    // kept in the fullscreen cvar so the settings overlay's Save to config keeps it.
    rex::ui::RegisterBind("bind_fullscreen", "F11", "Toggle fullscreen", [this] {
      if (rex::ui::Window* w = window()) {
        const bool on = !w->IsFullscreen();
        w->SetFullscreen(on);
        rex::cvar::SetFlagByName("fullscreen", on ? "true" : "false");
        REXLOG_INFO("fullscreen {} (bind_fullscreen)", on ? "on" : "off");
      }
    });
    futari::input_script::Install(runtime());
#if FUTARI_RENUT_RENDERER
    // Renut's renderer has no GPU: the guest's vertical blank comes from a thread of its own.
    if (banjo::renut_gpu::Active()) {
      futari::renut_vblank::Start();
      futari::capture::Start([](rex::ui::RawImage& image) { return renut::gpu::CaptureFrame(image, 100); },
                             [] { return rex::latency::SwapCount(); });
      return;
    }
#endif
    futari::capture::Start([this]() -> rex::ui::Presenter* {
      const auto* rt = runtime();
      // ReXGlue 0.8.0: graphics_system() is the IGraphicsSystem interface; the presenter is on GraphicsSystem.
      auto* graphics = rt ? static_cast<rex::graphics::GraphicsSystem*>(rt->graphics_system()) : nullptr;
      return graphics ? graphics->presenter() : nullptr;
    });
  }

  void OnShutdown() override {
    espgaluda2::exit_guard::ArmExitWatchdog();  // original code, src/exit_guard.h
    installer_.Stop();
    futari::capture::Stop();
#if FUTARI_RENUT_RENDERER
    if (banjo::renut_gpu::Active()) {
      futari::renut_vblank::Stop();
      renut::gpu::ShutdownRenderer();
    }
#endif
  }

#if FUTARI_RENUT_RENDERER
  // futari-nx futari_app.h: Renut's renderer draws the SDK's ImGui through its own plume drawer, and resizes its
  // swapchain itself.
  std::unique_ptr<rex::ui::ImmediateDrawer> OnCreateImmediateDrawer() override {
    if (banjo::renut_gpu::Active()) return renut::gpu::CreateOverlayDrawer();
    return rex::ReXApp::OnCreateImmediateDrawer();
  }
  void OnWindowPixelSizeChanged(uint32_t pixel_width, uint32_t pixel_height) override {
    rex::ReXApp::OnWindowPixelSizeChanged(pixel_width, pixel_height);
    if (banjo::renut_gpu::Active()) renut::gpu::OnWindowResized();
  }
#endif

  // futari-nx futari_app.h OnPostLoadXexImage: with ESPGALUDA2_DUMP_IMAGE=<file>, write the loaded default.xex image
  // (image_base..image_base+image_size), which tools/renut_dump_shaders.py reads the shader containers from.
  void OnPostLoadXexImage() override {
    const char* path = std::getenv("ESPGALUDA2_DUMP_IMAGE");
    if (!path || !runtime() || !runtime()->memory()) {
      return;
    }
    const uint8_t* image =
        runtime()->memory()->TranslateVirtual<const uint8_t*>(PPCImageConfig.image_base);
    if (std::FILE* f = std::fopen(path, "wb")) {
      std::fwrite(image, 1, PPCImageConfig.image_size, f);
      std::fclose(f);
      REXLOG_INFO("guest image written to {}", path);
    }
  }

 private:
  game_setup::IsoInstaller installer_;
};
