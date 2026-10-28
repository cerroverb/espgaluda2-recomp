# Espgaluda II Black Label Recomp

A native Linux x86-64 port of the Xbox 360 version of Espgaluda II Black Label,
statically recompiled with [ReXGlue](https://github.com/rexglue/rexglue-sdk).
It builds on the SDK copy bundled in
[SDOJ-Recomp](https://github.com/eandis/SDOJ-Recomp).

No game data is included. You need your own dump of the game.

## Status

- Boots and plays. Novice and 360 modes are tested, other modes less so.
- Plays through the first loop with sound and working slowdown.
- Tested on an AMD handheld (RADV) on the desktop and in Steam Game Mode.
  Intel and NVIDIA should work but are untested. Please open an issue if
  something is wrong.
- Slowdown accuracy compared to the arcade or the original 360 still needs
  checking by people who know the game well.

## Requirements

- An x86-64 Linux system with a Vulkan GPU driver (Mesa RADV/ANV, or NVIDIA).
- An extracted dump of Espgaluda II Black Label (Xbox 360). The folder must
  contain `default.xex`, `dat00.xex` ... `dat164.xex` and the `res` folder.
  To extract an ISO, use [extract-xiso](https://github.com/XboxDev/extract-xiso):
  `extract-xiso -x game.iso`.

## Install

1. Download `espgaluda2-recomp-linux-amd64.tar.gz` from Releases and unpack it.
2. Copy the contents of the extracted game folder into `game_data` (or replace
   `game_data` with a symlink to the folder).
3. Run `espgaluda2-recomp.sh`. For Steam, add it as a non-Steam game.

Saves and settings are written to `user_data` beside the executable.

## Settings

Press F4 in game for the ReXGlue settings overlay. Settings can also be given
on the command line, for example `--fullscreen=true`.

Changes in this port:

- **Input latency** (`latency_patch`, on by default): the game's D3D swap waits
  for its own frame instead of running one frame ahead of the GPU. Controller to
  screen time drops from about 32 ms to 16 ms at 60 fps.
- **Display-locked vblank** (`vsync_display_lock`, off by default): moves the
  game's 60 Hz timer in step with the frames the display actually shows. Only
  has an effect with a queued present mode (for example Steam Game Mode without
  Allow Tearing).

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

The code in this repository is under the BSD 3-Clause license (see LICENSE).
The ReXGlue SDK and Xenia carry their own licenses.
Espgaluda II is a trademark of CAVE. This project is not affiliated with CAVE.
