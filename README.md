<img width="1200" height="394" alt="78e74fe725904ecf96a7fc1f6df0c645" src="https://github.com/user-attachments/assets/b0448a33-493a-4b94-a095-5ba73c67b8b2" />

# Espgaluda II Black Label Recomp

A Linux x86-64 and AArch64 port of the Xbox 360 version of Espgaluda II Black Label,
statically recompiled with [ReXGlue](https://github.com/rexglue/rexglue-sdk).
It builds on the SDK copy bundled in
[SDOJ-Recomp](https://github.com/eandis/SDOJ-Recomp).

No game data is included. You need your own dump of the game.

## FAQ

### Windows/macOS?

Fork it if you want that, I'm not doing it. Linux AArch64 is out now (see Releases); BSD (NetBSD) though maybe. Or if enough shmuppers ask I'll consider a Windows version BIG MAYBE THOUGH.

### Is this a native port or just a glorified emulator?

Almost entirely native port. The game code (`default.xex` and the seven `dat*.xex`
modules) is recompiled ahead of time to C++, with no CPU emulation or JIT. The graphics are drawn by reNut's renderer
from the game's Direct3D calls with Vulkan, with no GPU emulation (`espgaluda2_renderer=xenos` for the emulated GPU).
The system calls are Xenia-derived reimplementations in C++.

The audio is the only emulated part left. The game streams its music in XMA, the Xbox 360's audio format, to the console's XMA
decoder chip, and the SDK emulates that chip (FFmpeg does the decoding).

### Is this AI slop?

Mostly no. The vast majority of this project is code ported and retooled from other recompilation projects, such as
futari-nx (the renderer port and its input-lag work), reNut (the Direct3D-level renderer), UnleashedRecomp (frame
pacing, GPU choice) and the ReXGlue SDK as bundled with SDOJ-Recomp, made to work for Espgaluda II on Linux. LLM
assistance was used in the porting, but with strict instructions to simply port and retool unless bespoke changes were necessary, which were made under my direct supervision, review and testing. I am a programmer recompilation is not my specialty and is still something I am learning about, thus the LLM assistance. There is also a strict no machine generated media rule, so you will never see slop generated "art" or media in regards to any project I administer.

### Does this port reduce input lag?

Yes, and this is on by default. The native renderer (the default) reads the pad as late as each frame allows and
presents right after the game's Swap: on a Steam Deck I measured 0.8-0.9 ms from pad read to present. With the
emulated renderer (`espgaluda2_renderer=xenos`) a patch removes about one frame: the game's Direct3D Swap saves the
GPU fence of the frame it just submitted and only waits for it at the next Swap, so the CPU runs a full frame ahead
of a GPU that flips on vblank. The patch makes Swap wait for its own frame's fence, using the game's own fence wait.
There I measured about 32 ms from pad read to present without the patch and about 16 ms with it. It can be turned
off with `--latency_patch=false`.

### What does vsync_display_lock do?

It applies to the emulated renderer only and is off by default. Normally the game's 60 Hz vblank timer runs on
its own, so a flip can land anywhere between two display refreshes. With
`--vsync_display_lock=true`, the timer follows when presented frames are actually
shown on screen, using `VK_KHR_present_wait`, and is nudged so each flip lands
just before a refresh. It needs a Vulkan driver with present wait
(`vulkan_present_wait`, on by default) and a queued present mode such as FIFO or
mailbox. With immediate presentation, or without present wait, it falls back to
the plain timer.

The native renderer has its own version, on by default (`futari_vblank_display_sync`). It is not vsync: vsync
(`renut_vsync`, off by default) decides whether a finished picture waits for the display's next refresh, while this
decides when the game starts each frame. The game's 60 Hz vertical blank (where it reads the pad and starts the frame)
follows the display's real refresh on a display at a whole multiple of 60 Hz (60, 120, 180, 240 Hz), and falls back to
the 60 Hz timer where the display's timing cannot be followed (for example through Xwayland under a desktop
compositor). The game runs at 60 frames a second either way. It works with vsync off: on a 240 Hz laptop display with
vsync off the game's vertical blank stayed locked to every fourth refresh at 60 fps.

### Will audio become native?

No existing project shows a native replacement yet that I could find, but let me know if one exists.

### Can this help with other CAVE games?

I hope so. It could be a starting point for other CAVE games on the Xbox 360,
alongside [SDOJ-Recomp](https://github.com/eandis/SDOJ-Recomp). How much code
those games share with this one still needs checking.

### What's next for this project?

Now that input latency and performance are in a good place, version 1.2.0 adds an English translation patch for the menus, messages and tutorials. The text is sourced from the official English of the Switch release. Where the Xbox 360 version has messages the Switch does not (Xbox LIVE, storage devices, Achievements), the closest official Switch lines are adapted. The tutorial's Japanese button labels are redrawn using the game's own English font. A few obvious errors in the official text are corrected, and every change is listed in a file that ships with the patch. The patch is on by default and can be turned off in the settings (`translation_patch`) to play with the original Japanese text.

Still in Japanese, and next on the list: the how-to-play captions shown after character select, before the first stage.

## Status

- Boots and plays all six modes (Black Label, Arrange, Arcade, Xbox 360 and both Novice modes) with sound and working
  slowdown.
- The native renderer (the default) holds 60 game updates a second, locked to the display's refresh on 60, 120 and
  240 Hz screens as a native Wayland window or under gamescope; through a desktop's Xwayland it keeps a 60 Hz timer. Vsync is off by default; on a Steam Deck the
  pad read reaches the present in 0.8-0.9 ms.
- Tested on:
  - an AMD handheld (RADV) on the desktop and in Steam Game Mode;
  - a Steam Deck (RADV), headless runs of every mode and of 120 and 240 Hz displays;
  - a laptop with an NVIDIA RTX 5080 (driver 615) on KDE Plasma Wayland at 240 Hz.
  Intel GPUs should work but are untested. Please open an issue if something is wrong.
- Slowdown accuracy compared to the arcade or the original 360 still needs checking by people who know the game well. I compared them against xenia-edge and my hardmodded xbox360 with my own physical copy of the game and it seems pretty accurate, but I don't have the best eye for this kind of thing, so more eyes would be great.
- Certain things could use further testing. Two players and replays, for example, so feel free to shit up the issues section with any problems you have. 

## Requirements

- An x86-64 or AArch64 (ARM64) Linux system with a Vulkan GPU driver (Mesa RADV/ANV/Turnip, or NVIDIA); the AArch64
  build needs glibc 2.38 or newer.
- Your own copy of Espgaluda II Black Label (Xbox 360), either as an `.iso`
  or as extracted files (`default.xex`, `dat00.xex` ... `dat164.xex` and the
  `res` folder).

## Install

1. Download `espgaluda2-recomp-linux-amd64.tar.gz` (x86-64) or `espgaluda2-recomp-linux-arm64.tar.gz` (AArch64)
   from Releases and unpack it.
2. Add the game, one of two ways:
   - **ISO:** put the `.iso` next to `espgaluda2-recomp.sh`. The first launch
     extracts it into `game_data` (a progress window shows while it runs),
     after which the ISO can be deleted.
   - **Extracted files:** copy them into `game_data`, or make `game_data` a
     symlink to the folder.
3. Run `espgaluda2-recomp.sh`. For Steam, add it as a non-Steam game.

Saves and settings are written to `user_data` beside the executable.

## Settings

Press F4 in game for the settings window (search box at the top; "Save to
config" keeps them in `espgaluda2_recomp.toml` next to the game). Settings can
also be given on the command line, for example `--fullscreen=true`. Every
setting with its default is in [docs/settings.md](docs/settings.md).

The ones changed by this port (native renderer, the default):

- `renut_vsync` (off by default): off shows each picture at once, the least
  input lag, tearing possible; on waits for the display's refresh.
- `futari_vblank_display_sync` (on by default): the game's 60 Hz vertical blank
  follows the display's refresh (see the FAQ above).
- `translation_patch` (on by default): the English translation patch, the
  official English of the game's Switch release for the menus, messages and
  tutorials (see `translation/README.txt`). Off shows the original Japanese.
  Applies at the next launch.
- `latency_patch` (on by default): the game's swap waits for its own frame
  instead of running one frame ahead of the GPU.
- `futari_fill_crop` (off by default): on a 16:10 or 4:3 screen, on fills the
  screen by trimming only the wallpaper at the sides; off shows black bars.
- `renut_resolution`: the internal resolution, 720p (the game's own) by default.

In Steam Game Mode, turn on **Allow Tearing** in the quick menu (Performance) to
get presentation without vsync. Without it, gamescope forces mailbox mode.

## Building

You need CMake 3.25 or newer, Ninja, Clang with C++23 support (Clang 20 or newer) and git.
On Debian/Ubuntu also:

```
sudo apt install libgtk-3-dev libx11-xcb-dev libvulkan-dev libasound2-dev libpulse-dev libpipewire-0.3-dev libcurl4-openssl-dev
```

On Arch:

```
sudo pacman -S --needed cmake ninja clang git gtk3 vulkan-headers vulkan-icd-loader alsa-lib libpulse pipewire curl
```

Then run:

```
./build.sh
```

The first configure downloads SDOJ-Recomp at a pinned commit and applies the
patches in `sdk-patches`. The build uses a lot of memory because the generated
code is large.

The recompiled code in `generated` is committed. To regenerate it, put the
game's `.xex` files in a `disc` folder and build the `espgaluda2_recomp_codegen`
target.

## License

The code in this repository is under the BSD 3-Clause license (see LICENSE), except the renderer ported from
futari-nx (`src/renut_engine`, `src/futari_*`, the files marked as ported), which is GPL-3.0 (LICENSE-GPL-3.0);
a binary built with it is GPL-3.0 as a whole.
The ReXGlue SDK, Xenia and extract-xiso (bundled in `tools/`) carry their own licenses.
Espgaluda II is a trademark of CAVE. This project is not affiliated with CAVE.
