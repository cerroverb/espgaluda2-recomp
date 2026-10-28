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
                      screen is not 16:9. false: stretched to fill the screen.
  renut_vsync         true (default). false: no vsync (less lag, tearing).

The same settings work as launch options or in the file, for example:
  ./espgaluda2-recomp.sh --renut_resolution=1440p --fullscreen=true
or in espgaluda2_recomp.toml:
  renut_resolution = "1440p"
  fullscreen = true
