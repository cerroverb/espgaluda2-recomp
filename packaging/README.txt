Espgaluda II Black Label Recomp (Linux x86-64)

1. Add your Xbox 360 copy of the game, either:
   - put the .iso next to espgaluda2-recomp.sh (the first launch extracts it
     into game_data, then the ISO can be deleted), or
   - copy the extracted game files (default.xex, dat*.xex, res, ...) into
     the game_data folder.
2. Run espgaluda2-recomp.sh. For Steam, add it as a non-Steam game.

Saves go to user_data. In Steam Game Mode, turn on Allow Tearing (quick menu,
Performance) for no vsync.

Display settings
----------------
F4 opens the settings (type a name into the search box at the top, e.g.
"resolution"); "Save to config" keeps them in espgaluda2_recomp.toml next to
the game. F11 switches fullscreen on and off.

  renut_resolution    Internal resolution: 480p, 720p (the game's own,
                      default), 1080p, 1440p or 4K. Applies at once.
                      Pick the one nearest your screen's height:
                      1280x800 -> 720p, 1920x1080 -> 1080p,
                      2560x1440 or 2560x1600 -> 1440p, 3840x2160 -> 4K.
  fullscreen          true (default): fullscreen at the screen's own size.
  present_letterbox   true (default): the whole picture, black bars where the
                      screen is not 16:9. The native renderer always keeps
                      the picture's shape; false stretches it only with
                      the emulated renderer (espgaluda2_renderer=xenos).
  renut_vsync         true (default). false: no vsync (less lag, tearing).

The same settings work as launch options or in the file, for example:
  ./espgaluda2-recomp.sh --renut_resolution=1440p --fullscreen=true
or in espgaluda2_recomp.toml:
  renut_resolution = "1440p"
  fullscreen = true

Graphics card (laptops with two GPUs)
-------------------------------------
The game uses a dedicated GPU if there is one, else the built-in one. The log
(logs/ next to the game) lists every GPU on a line starting "GPUs:" and the one
in use on "Renderer: Vulkan on ...". To pick another one, copy its name from
that line exactly:
  ./espgaluda2-recomp.sh --espgaluda2_gpu="NAME FROM THE LOG"
(or espgaluda2_gpu = "NAME" in espgaluda2_recomp.toml; restart to apply).
The older emulated renderer (--espgaluda2_renderer=xenos) uses
--vulkan_device=N instead (N = 0, 1, ... in the order Vulkan lists them).

Checking the game's speed
-------------------------
--espgaluda2_rate_counter=true (or switch it in F4) writes a line to the log
every second: game updates, guest vblanks and presents a second, and the
display's refresh rate. The game should show 60 updates a second on any
display (also at 120 or 144 Hz).
