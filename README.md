# Espgaluda II Black Label Recomp

A Linux x86-64 port of the Xbox 360 version of Espgaluda II Black Label,
statically recompiled with [ReXGlue](https://github.com/rexglue/rexglue-sdk).
It builds on the SDK copy bundled in
[SDOJ-Recomp](https://github.com/eandis/SDOJ-Recomp).

No game data is included. You need your own dump of the game.

## FAQ

### Windows/macOS?

Fork it if you want that, I'm not doing it. Linux aarch64 and BSD (NetBSD) though maybe.

### Is this a native port?

The game code is: the PowerPC code of `default.xex` and the seven `dat*.xex` modules is recompiled ahead of time into
C++ and runs directly on x86-64, with no CPU emulation or JIT. Graphics are native by default too: the game's
Direct3D calls are drawn by reNut's Direct3D-level renderer on Vulkan, with no Xbox 360 GPU emulation (the emulated
GPU stays available with `espgaluda2_renderer=xenos`). Audio still goes through emulated XMA decoding, and the Xbox
kernel and XAM calls are Xenia-derived reimplementations in C++.

### Is this AI?

Mostly no. The vast majority of this project is code ported and retooled from other recompilation projects, such as
futari-nx (the renderer port and its input-lag work), reNut (the Direct3D-level renderer), UnleashedRecomp (frame
pacing, GPU choice) and the ReXGlue SDK as bundled with SDOJ-Recomp, made to work for Espgaluda II on Linux. LLM
assistance (Claude) was used in the porting.

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

The native renderer has its own version, on by default (`futari_vblank_display_sync`): the game's vertical blank
follows the display's refresh on 60, 120 and 240 Hz screens, and falls back to the 60 Hz timer where the display's
timing cannot be followed (for example through Xwayland under a desktop compositor).

### Will audio become native?

Audio still uses the SDK's XMA decoding: the game streams its music through the Xbox 360's XMA decoder, which the SDK
reproduces (the decoding itself is FFmpeg's), and no existing project shows a native replacement yet.

### Can this help with other CAVE games?

I hope so. It could be a starting point for other CAVE games on the Xbox 360,
alongside [SDOJ-Recomp](https://github.com/eandis/SDOJ-Recomp). How much code
those games share with this one still needs checking.

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
- Your own copy of Espgaluda II Black Label (Xbox 360), either as an `.iso`
  or as extracted files (`default.xex`, `dat00.xex` ... `dat164.xex` and the
  `res` folder).

## Install

1. Download `espgaluda2-recomp-linux-amd64.tar.gz` from Releases and unpack it.
2. Add the game, one of two ways:
   - **ISO:** put the `.iso` next to `espgaluda2-recomp.sh`. The first launch
     extracts it into `game_data` (takes a few seconds), after which the ISO
     can be deleted.
   - **Extracted files:** copy them into `game_data`, or make `game_data` a
     symlink to the folder.
3. Run `espgaluda2-recomp.sh`. For Steam, add it as a non-Steam game.

Saves and settings are written to `user_data` beside the executable.

## Settings

Press F4 in game for the ReXGlue settings overlay. Settings can also be given
on the command line, for example `--fullscreen=true`.

Changes in this port:

- **Input latency** (`latency_patch`, on by default): the game's D3D swap waits
  for its own frame instead of running one frame ahead of the GPU. This cuts
  about one frame of input lag.
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

The code in this repository is under the BSD 3-Clause license (see LICENSE), except the renderer ported from
futari-nx (`src/renut_engine`, `src/futari_*`, the files marked as ported), which is GPL-3.0 (LICENSE-GPL-3.0);
a binary built with it is GPL-3.0 as a whole.
The ReXGlue SDK, Xenia and extract-xiso (bundled in `tools/`) carry their own licenses.
Espgaluda II is a trademark of CAVE. This project is not affiliated with CAVE.
