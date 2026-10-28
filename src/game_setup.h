// First-launch game data setup: finds the game files, or extracts an ISO
// into game_data with the bundled extract-xiso.

#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace rex::ui {
class ImGuiDrawer;
class WindowedAppContext;
}  // namespace rex::ui

namespace game_setup {

// Returns the folder that holds default.xex: game_data itself, or a single
// subfolder of it (when a whole game folder was copied in).
std::optional<std::filesystem::path> FindGameFolder(const std::filesystem::path& game_data);

class IsoInstaller {
 public:
  using Done = std::function<void(bool ok, const std::string& error)>;

  IsoInstaller();
  ~IsoInstaller();

  IsoInstaller(const IsoInstaller&) = delete;
  IsoInstaller& operator=(const IsoInstaller&) = delete;

  // Looks for exactly one .iso beside the executable or inside game_data and
  // extracts it into game_data on a worker thread, showing progress. `done`
  // runs on the UI thread.
  void Start(rex::ui::WindowedAppContext& context, rex::ui::ImGuiDrawer* drawer,
             const std::filesystem::path& exe_dir, const std::filesystem::path& game_data,
             Done done);
  void Stop();

  struct State;

 private:
  std::shared_ptr<State> state_;
};

}  // namespace game_setup
