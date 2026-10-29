# Aspect ratio report: 16:10, 21:9, 4:3 (assessment only)

Date 2026-10-29. Nothing was built or changed for this report. Each finding is marked VERIFIED (with a file:line,
an offset or a measurement) or INFERRED (reasoned, not checked by running).

## Plain summary

- The game draws one fixed 16:9 picture: a wallpaper image (1280x720 art) as the background, and the play field pasted
  on top of it as one rectangle. The play field (with its arcade score display) is drawn separately first, then placed
  in the middle.
- The wallpaper art only exists at 16:9. A 16:10, 21:9 or 4:3 screen can only be filled edge to edge by cutting
  some of the wallpaper away, stretching it, or painting new art. The owner rules out all three. **So true full-screen
  16:10 / 21:9 / 4:3 is not possible without new art or cropping.** This is the main finding.
- None of the options below change the play field itself. The current black bars (letterbox) already show the whole
  original picture, unstretched, and should stay the default.
- On the common screens the bars give a clean result: 2560x1600 and 1280x800 (16:10) and 2560x1080 / 3440x1440 (21:9)
  all show the arcade pixels at an exact whole-number size.
- The game has its own screen options (wallpaper choice from about 60 images, play field position and zoom, rotation
  for vertical "TATE" monitors, extra score windows). Picking a dark wallpaper makes the bars blend in, with no code.
- A small possible addition: fill the bars with a blurred, darkened copy of the picture instead of black. It touches
  only the final display step, never the game picture. No existing project here has it, so it would be new code.
- Not recommended: anything that widens or heightens the game's own canvas. It needs new art and reverse engineering
  of the menus and layout, and it changes the 360 screen layout around the play field.

## 1. How the game builds its picture

| Finding | Status | Evidence |
|---|---|---|
| Sprites are positioned in **pixel coordinates**. The sprite vertex shader is `oPos.x = (Pos.x / g_ScreenX) - 1; oPos.y = 1 - (Pos.y / g_ScreenY)`, with no matrix. | VERIFIED | HLSL text in the decrypted default.xex image, offset 0x8DDBD9 (`Out.oPos.x = (v.Pos.x/g_ScreenX)-1.0f;`), technique `TShader`, `game:\res\shader\sprite*.fx` names nearby. Decrypted with tools/xex/xex_image.py (scratch copy, not in the project). |
| A second embedded effect uses `g_mWorldMatrix`/`g_mViewProj` (a 3D mesh / matrix sprite path). No draw in the logged gameplay frame needs it for the layout. | VERIFIED (text present), INFERRED (that layout does not use it) | same image, the effect before the pixel-coordinate one |
| The game's screen size is two globals (0x829366B0 width = 1280, 0x829366AC height = 720). The game copies them from the video mode (`XGetVideoMode`) and clamps them to at most 1280x720. Half of each becomes `g_ScreenX`/`g_ScreenY`. | VERIFIED (clamp, globals), INFERRED (the halves feed the shader constants) | generated/default/espgaluda2_recomp_recomp.3.cpp:61521 `sub_8210B1A0` (calls 0x8237B3BC = `__imp__XGetVideoMode`, `cmplwi r9,1280` at :61560, then 720); recomp.0.cpp:34142 `sub_8209DFF0` stores 1280/720; recomp.1.cpp:7802-7823 (`sub_820B4F20`) loads them and halves them (`srawi ...,1`) |
| The play field is drawn into its own **640x544 render target**, then pasted into the 1280x720 back buffer with **one quad** (6 indices). The wallpaper/side art goes straight into the back buffer before it (2 draws, 4 + 6 quads, from 1024x1024 textures). | VERIFIED | out/compare/drawlog_2694.log (swap 2694): draws 0-37 target 640x544; draws 38, 39 target 1280x720, textures 1024x1024 (fetch dword2 0x007FE3FF); draw 40 target 1280x720, 6 indices, texture 640x544 (dword2 0x0043E27F = width 640, height 544) |
| The arcade HUD (PLAYER 1, HIGH score) is inside the play field layer, not on the back buffer. | VERIFIED (screenshot), INFERRED (layer) | out/compare/c2694_0.png: HUD text at y 40-70 inside the field rectangle; only 3 back-buffer draws exist |
| On the 1280x720 frame the field is exactly **480x640 at (400, 40)**: 2x the arcade's 240x320. | VERIFIED (measurement) | c2694_0.png pixel edges: x 400 and 880, y 40/41 and 680 |
| The field target is drawn landscape (640 wide) and turned 90 degrees when pasted, 1:1 (640 rows -> 640 tall, 480 of 544 columns -> 480 wide). | INFERRED | 640x544 cannot hold a 480x640 portrait image without uneven scaling; consistent with the CV1000 board's 320x240 landscape output and the game's "Rotate" option |
| The wallpaper is a 16:9 picture (one per file, res/DISKDATA/SYS/xbox3*.bin, `wall_paper_01.tga` etc., about 1 MB compressed each) with no hidden extra rows or columns. | INFERRED | file headers name one .tga each; the art fills 1280x720 exactly in every screenshot. Not decoded (compressed container). Check: `banjo_renut_diag_show_texture` on the 1024x1024 textures of draws 38/39, or the UVs of those draws |
| Renut currently shows the whole 1280x720 frame, centred, whole-pixel edges, black bars, never stretched. | VERIFIED | src/renut_engine/gpu/backend/present.cpp:120-144 `FitFrame`; WORKLOG.md "Task B" table (2560x1600 -> 2560x1440 at (0,80); 2560x1080 -> 1920x1080 at (320,0)) |
| Renut's guest screen width is a constant 1280 (no widescreen hooks in this project). | VERIFIED | src/renut_engine/gpu/render/graphics_options.cpp:33-35; src/renut_engine/gpu/guest/widescreen.h |

Result: the picture is built from three layers: wallpaper (back buffer), play field with the arcade HUD (its own
target), and the paste of the field. Every layer uses fixed pixel positions in a 1280x720 space. No projection could
"show more" around the edges. The only thing that could fill a wider or taller screen is more wallpaper, and that art
does not exist.

## 2. The game's own screen options

VERIFIED: these option strings exist in the decrypted default.xex. Offsets are in the image; VA = 0x82000000 + offset.
INFERRED: what each one does. It is read from the names only, not checked in the running menu.

- OPTION > Screen / "General Screen" (0x8358): `Position X`, `Position Y`, `Zoom`, `Zoom X`, `Zoom Y`, `Smoothing`,
  `Brightness`, `Rotate` (0x7988), `Up Side Down`, `Link Rotation`. Probably the play field's place and size on the
  16:9 frame, a TATE (rotated monitor) mode and filtering. `Zoom X`/`Zoom Y` separately would allow stretching. That is
  the player's choice in the game, not ours.
- Wallpaper: `Wallpaper 1-1`, `Wallpaper 1-2`, `Wallpaper 2` ... `Wallpaper 59`, `Wall0000`, `BG Change`,
  `BG Change Time` (30sec ... 30min, Custom). About 60 selectable 16:9 wallpapers, optionally cycling. The data files
  are res/DISKDATA/SYS/xbox301-322, 3a01-02, 3b01-05, 3c01-07, 3d01-23 (59 files).
- "Extra Window Setting" (0x786C): `Input Display`, `Score Detail`, `Score Disp`, `High Score Disp`, `Score Position X/Y`,
  `High Score Position X/Y`, `1P/2P Position X/Y`, `Show Window`, `Hide Window`, `Change Window`, `Preset 1`.
  Extra information windows that the player places in the side area.
- No 4:3 or "aspect" option was found (searched: aspect, wide, 4:3, 16:9, letterbox). No string search was done in the
  dat*.xex modules.

These options already let the player change the 360 layout around the arcade field. With a dark wallpaper, letterbox
bars are hardly visible. Neither needs any code.

## 3. Existing projects as port sources

### reNut (Banjo-Kazooie: Nuts & Bolts): ~/projects/reNut/src/renut_engine/gpu/guest/widescreen.cpp

- What it does (VERIFIED, read): cvar `renut_aspect_ratio` (Auto, 16:9, 21:9, 32:9) at :20. "Narrower than 16:9 stays
  16:9", so it has **no 16:10 or 4:3 support**, only wider screens. `WidthForAspect` (:104) keeps height 720 and widens
  the guest screen. Hooks at N&B addresses: the video-mode size query `sub_823BB7C0` (:243) returns the wider width;
  screen pools and scratch textures are resized (:254, :266); the 3D camera's projection aspect is widened (:169,
  :274, :281); XUI menus stay 1280 wide and are centred (:194, :288-:341). Used by render/graphics_options.cpp and
  surfaces.cpp through `GuestScreenWidth()`.
- Fit for Espgaluda: only the idea carries over. The `sub_823BB7C0` hook corresponds to Espgaluda's `XGetVideoMode`
  clamp in `sub_8210B1A0`. Everything else is about a 3D camera and XUI, which Espgaluda does not use. A wider guest
  screen here would still have nothing to draw in the extra width (see section 1).

### UnleashedRecomp (Sonic Unleashed): ~/projects/UnleashedRecomp/UnleashedRecomp/

- gpu/video.cpp:3015-3063 `Video::ComputeViewportDimensions`: option `AspectRatio` (Auto, Wide, Narrow,
  OriginalNarrow; user/config.h:83, config_def.h:58). Wide/Narrow make a fixed 16:9 or 4:3 viewport with bars. Auto
  renders at the window's own aspect.
- patches/aspect_ratio_patches.cpp:185-220 `ComputeOffsets`: offsets and scales for the 2D layer. Below 4:3 it crops.
  :308-350 has the per-element flags (ALIGN_*, STRETCH, EXTEND_LEFT/RIGHT, SCALE ...). :367 onwards holds a table of
  371 hand-made entries keyed by the names of Sonic Unleashed's CSD UI scenes. These say how each HUD piece is anchored
  or extended, and some background pieces are simply `STRETCH`ed.
- patches/camera_patches.cpp:18-30: the 3D field of view is kept "Vert+" for narrow screens.
- patches/video_patches.cpp:9-36: fits movies to the screen aspect. ui/black_bar.cpp: pillarbox bars for cutscenes and
  loading screens.
- Fit for Espgaluda: the Wide setting is what Renut already does. Auto works for Unleashed because its world is 3D
  (more scene can be rendered) and its 2D pieces are named and anchored by a hand-made table. Espgaluda's side area is
  one flat 16:9 painting, so Auto would need new art or stretching (Unleashed itself stretches some backgrounds).
  Nothing portable for the play field. The idea of an Auto/Wide/Narrow option name and a black-bar overlay could be
  reused.

### futari-nx / futari-linux (Mushihimesama Futari, same CAVE 360 engine family)

- futari-linux/app/src/renut_engine/gpu/guest/widescreen.h and render/graphics_options.cpp:32: the guest screen is
  1280x720 always (VERIFIED). futari-nx is the same.
- futari-linux/app/src/renut_engine/gpu/backend/present.cpp:74, :117-:165: `futari_fill_crop` (default false) fills
  the screen by trimming up to 15 % (`kMaxTrim`) of the picture's edges. Off gives letterbox. futari-nx's present.cpp:108
  always trimmed, which is why espgaluda2 replaced it (WORKLOG.md Task B: on 16:10 it cut the logo at the left).
- futari-nx/docs/futari-notes.md:15, :108: Futari has the same structure: wallpapers (media/wall/wp01-13.bin) and a
  centre panel drawn into the 1280x720 frame.
- futari-linux WORKLOG.md sections 16 and 19-23: the same owner rules applied there (letterbox default, crop only as an
  option). Its letterbox was later ported back from the SDK presenter, the same code espgaluda2 uses now.
- Fit: the crop option is the only "fill" either project has. It breaks the owner's no-crop rule, so it should only be
  offered as an opt-in, if at all.

### rexglue SDK presenter: ~/projects/futari-linux/sdk/src/ui/presenter.cpp

- `present_letterbox` (true) :153, `present_safe_area_x/y` (90) :156-:162, `present_allow_overscan_cutoff` (false)
  :205. `GetPaintFlowForGuestOutput` :1035-:1110: fit by aspect. If overscan cutoff is allowed, up to (100 - safe area) %
  may be cut, otherwise letterbox. With letterbox off, it stretches. No integer scaling, no bar fill, no rotation.
- This is already ported: present.cpp `FitFrame` is this rule with safe area 100 (WORKLOG.md "Ports from
  futari-linux").

### Also seen (not asked): ~/projects/reblue

src/gpu/output.h and settings.h have an aspect setting with a "design canvas" fit (`ComputeFit`, `ProjectionAspect`).
It was not examined further. It is a 3D game too.

## 4. Approaches per aspect ratio

Field pixel scale in this table = how many screen pixels one arcade pixel gets (field is 2x arcade at 720p).

| Screen | Letterbox today | Field scale |
|---|---|---|
| 1280x800 (Deck) | 1280x720, 40 px bars top/bottom | 2x, exact |
| 1920x1200 | 1920x1080, 60 px bars | 3x, exact |
| 2560x1600 | 2560x1440, 80 px bars | 4x, exact |
| 2560x1080 (21:9) | 1920x1080, 320 px bars left/right | 3x, exact |
| 3440x1440 (21:9) | 2560x1440, 440 px bars | 4x, exact |
| 1600x1200 (4:3) | 1600x900, 150 px bars | 2.5x, not exact |
| 1024x768 (4:3) | 1024x576, 96 px bars | 1.6x, not exact |

(Computed from FitFrame's rule; 1280x800, 2560x1600 and 2560x1080 also measured in WORKLOG.md Task B. An exact scale
needs `renut_resolution` matching the output height (e.g. 1080p for 1920x1200), otherwise the frame is resampled.)

### A. Letterbox / pillarbox only (current default)

- 16:10, 21:9, 4:3: the whole frame, unstretched, uncropped. The play field and arcade layout are unchanged.
- Code: none (present.cpp:120-144). Risk: none. Must stay the default and never block a release.

### B. Letterbox with filled bars (blurred/darkened copy of the frame, or a solid colour from the wallpaper)

- No stretch, squash or crop of the game picture. The fill is decoration outside it. The play field is untouched.
  Note: the fill is made from the picture itself, so it is not new art, but it is something the arcade or 360 never
  showed. It must stay optional.
- Port source: none found in reNut, UnleashedRecomp (only black bars, ui/black_bar.cpp), futari-nx/linux or the SDK
  presenter. New code: present.cpp draws the frame texture once more, scaled to cover the screen, blurred or darkened,
  then the letterboxed frame on top. About 100-200 lines plus a small shader. Risks: GPU cost on the Deck (a blur at
  output size), the edges of the wallpaper repeated in the blur. It works the same for all three ratios.

### C. Whole-number field scale option (show the frame at an integer factor)

- No stretch or crop. Bars get bigger where the factor rounds down. The play field is unchanged.
- Already exact on the common 16:10 and 21:9 sizes (table). Only helps 4:3 and odd sizes, for example 1600x1200: frame
  at 1x = 1280x720 with large bars, or keep 2.5x.
- Port source: none in the listed projects (the SDK presenter has no integer mode). New code: about 10-20 lines in
  FitFrame plus a cvar. Risk: low. The frame gets smaller on 4:3.

### D. The game's own options (Zoom / Position / Wallpaper / Extra windows)

- In-game choices the 360 version already offers. A dark wallpaper hides the bars. Zoom makes the field larger inside
  the 16:9 frame. Zoom X/Y separately would stretch (the player's choice).
- Zoom changes the **360 layout** (field size against the wallpaper), not the arcade field's content. No code.
  Risk: none for us. The effects are inferred from the names only.

### E. Wider or taller game canvas (the reNut/Unleashed way): e.g. guest back buffer 1280x800 for 16:10, 1680x720 for 21:9, 960x720 or 1280x960 for 4:3

- Field: can stay 480x640 and centred. The play field itself does not need to change.
- Side area: the extra area has nothing to show. Filling it means **new art** (extending each of about 60 wallpapers),
  **cropping/scaling** the wallpaper (for example 960x720 for 4:3 cuts 160 px from each side; scaling a 16:9 wallpaper
  to cover 1280x800 cuts about 71 px from each side), or **stretching** it. All three are against the owner's rules.
  The only remaining choices are an empty, black or blurred extra area, and that looks like option B with much more work.
- Work: hook the clamp in `sub_8210B1A0` (like reNut's `sub_823BB7C0` hook) or the screen-size globals. Find out
  whether the field position, the wallpaper quads and the extra windows follow the globals or fixed numbers (not
  known). Make Renut size the back buffer from it (graphics_options.cpp `GuestScreenWidth`, surfaces). Check every
  menu, loading screen, ending and the replay/attract screens. Probably weeks, and new art on top. Risks: menus drawn at
  fixed 1280x720 positions end up off-centre; the game's own "Position X/Y" ranges; saves with stored positions.
- The extra info windows (score detail, input display) could perhaps be placed in a 21:9 side area. That would be new
  layout, not arcade layout.

### F. Crop to fill (futari-linux `futari_fill_crop`, SDK overscan cutoff)

- 16:10: cuts 10 % of the width (64 px of wallpaper per side at 720p). The field (x 400-880) is not touched. 4:3 needs
  25 % of the width (160 px per side), more than the 15 % limit. A full 4:3 crop would still miss the field, but it
  removes most of the side art. 21:9: the crop removes top and bottom rows (720 -> 540), so it **would cut the play
  field** (640 rows tall). **Excluded by the owner's rules.** It could be ported in an hour as an opt-in. Not
  recommended.

### G. TATE / portrait (beyond the three asked; "possibly others")

- The game has `Rotate`, `Up Side Down` and `Link Rotation` (section 2). On a monitor turned to portrait while the
  desktop stays landscape, this should work today with no code (INFERRED). On a desktop rotated to portrait (window
  taller than wide), the 16:9 frame is letterboxed very small. A "rotate output 90 degrees" option in present.cpp
  (UV swap, about 20-40 lines, new code) together with the game's Rotate would give the full-height field. Not
  requested. It is listed because it is the one non-16:9 case where the arcade field could become bigger without new
  art.

## 5. Recommendation

Possible without touching the play field or the arcade layout:
1. Keep letterbox/pillarbox as the default for 16:10, 21:9 and 4:3 (done, present.cpp:120-144). On 2560x1600,
   1920x1200, 1280x800, 2560x1080 and 3440x1440 the arcade pixels are already at an exact whole-number scale. Release
   as is.
2. Optional, small: a bar fill (approach B) and/or a whole-number scale switch (approach C). Both are presentation-only.
   No port source exists in the listed projects, so both would be new code. Ask the owner before writing it.
3. No code: document the game's own Wallpaper/Zoom/Position options in packaging/README.txt (a dark wallpaper makes
   the bars almost invisible). First check them in the running menu, since their effect here is inferred from names.

Not possible without breaking a rule:
- True full-screen 16:10, 21:9 or 4:3 (no bars) always needs one of: new wallpaper art, cropping the wallpaper, or
  stretching it (approaches E and F). The play field itself could stay as it is, but the 16:9 wallpaper cannot fill a
  non-16:9 screen. **Cropping, stretching and changing the visible play field are unavoidable for a bar-free picture
  unless new art is made.** I do not recommend approach E: it needs reverse engineering, weeks of work and new art for
  about 60 wallpapers.

Open checks (cheap, need a run, not done because this was assessment only):
- Confirm the wallpaper textures hold no extra rows or columns (`banjo_renut_diag_show_texture` on draws 38/39 of the
  drawlog).
- Confirm in the running menu what Zoom / Position / Rotate / Wallpaper do, and whether a black or plain wallpaper
  exists.
