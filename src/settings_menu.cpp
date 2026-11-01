// Original code; see settings_menu.h.

#include "settings_menu.h"

#include <string>
#include <string_view>
#include <unordered_map>

#include <rex/cvar.h>
#include <rex/logging.h>

namespace espgaluda2::settings_menu {

namespace {

// The settings shown in the window, by name, with their category there.
const std::unordered_map<std::string_view, std::string_view> kShown = {
    // Display
    {"fullscreen", "Espgaluda2/Display"},
    {"monitor", "Espgaluda2/Display"},
    {"window_width", "Espgaluda2/Display"},
    {"window_height", "Espgaluda2/Display"},
    {"renut_resolution", "Espgaluda2/Display"},
    {"renut_vsync", "Espgaluda2/Display"},
    {"futari_fill_crop", "Espgaluda2/Display"},
    {"renut_anisotropic_filtering", "Espgaluda2/Display"},
    {"espgaluda2_gpu", "Espgaluda2/Display"},
    {"espgaluda2_renderer", "Espgaluda2/Display"},
    // Input lag and frame timing
    {"latency_patch", "Espgaluda2/Latency"},
    {"futari_vblank_display_sync", "Espgaluda2/Latency"},
    {"futari_vblank_phase_auto", "Espgaluda2/Latency"},
    {"futari_vblank_phase_max_us", "Espgaluda2/Latency"},
    {"futari_vblank_delay_us", "Espgaluda2/Latency"},
    {"futari_vblank_wait_shown", "Espgaluda2/Latency"},
    {"futari_renut_double_buffer", "Espgaluda2/Latency"},
    {"banjo_late_input", "Espgaluda2/Latency"},
    {"input_frame_read_delay_us", "Espgaluda2/Latency"},
    // Game
    {"translation_patch", "Espgaluda2/Game"},
    // Audio
    {"audio_mute", "Espgaluda2/Audio"},
    {"audio_maxqframes", "Espgaluda2/Audio"},
    // Keyboard
    {"mnk_mode", "Espgaluda2/Keyboard"},
    // Logs
    {"log_level", "Espgaluda2/Logs"},
    {"espgaluda2_rate_counter", "Espgaluda2/Logs"},
    // Program key bindings (the SDK's bind_debug_overlay and bind_console stay hidden)
    {"bind_settings", "Espgaluda2/Keybinds/System"},
    {"bind_fullscreen", "Espgaluda2/Keybinds/System"},
};

// The keyboard's controller bindings (keybind_*) move as a group.
constexpr std::string_view kControllerKeybinds = "Input/Keybinds/Controller";

}  // namespace

void Apply() {
  size_t shown = 0, hidden = 0;
  for (rex::cvar::FlagEntry& entry : rex::cvar::GetRegistry()) {
    if (const auto it = kShown.find(entry.name); it != kShown.end()) {
      entry.category = std::string(it->second);
      ++shown;
    } else if (entry.category == kControllerKeybinds) {
      entry.category = "Espgaluda2/Keybinds/Controller";
      ++shown;
    } else {
      entry.is_debug_only = true;
      ++hidden;
    }
  }
  REXLOG_INFO("settings window: {} settings shown, {} hidden (src/settings_menu.cpp)", shown, hidden);
}

}  // namespace espgaluda2::settings_menu
