# NativeXRenderer in Guitar Hero 5

The game's side of NativeXRenderer (`native_x_renderer/`, the library that hooks a game's Direct3D calls and draws
on the host). The hooks and the profile are in [`src/native_x_renderer_gh5.cpp`](../src/native_x_renderer_gh5.cpp);
the library's checklist is `native_x_renderer/docs/integrating-a-game.md`.

**Status (2026-11-06):** GH Van Halen's side (GH Metallica's map), translated. The default renderer; the launcher's
Xenos card or `--native_x_renderer=false` turns it off.

## The addresses

- GH5 links the XDK Direct3D library of 2.0.8276 (GH Van Halen 2.0.7978) at `0x8230A8D0`–`0x8232A5AC`: 561 of GH Van
  Halen's 575 library functions are the same instruction for instruction once data addresses are masked; Present is
  recompiled (other registers and scheduling) with the same logic and device offsets. So GH Metallica's map
  (GHMetalRecomp's `docs/native-x-renderer-ghmetallica.md`: the device layout, Dante's Inferno's up to the register
  shadow; the current objects POTCAWE's + 144; the resource objects POTCAWE's) holds, with every field checked in 18
  functions.
- Each of the 36 hooked functions found by the function matcher and checked side by side; the draws at
  `0x8230BEC8`–`0x8230D198`, the XGraphics-style helpers at `0x823123C8`–`0x82313EE8`.
- The device pointer is at `0x82D991B0`: the renderer's block `0x82D989C0` + 2032 (GH Van Halen's + 2292).
- The engine has three BeginVertices / EndVertices pairs (`sub_822FE350`, `sub_822FFA78`, `sub_82303C00`); as in GH
  Van Halen, `inline_draw_end = 13444` covers them and EndVertices is not hooked.
- **The scene's copy, for SMAA and FXAA:** `scene_resolve_caller = 0x82571324`, `scene_resolve_outer_caller =
  0x82571A64`, `scene_resolve_once = true`. See [Antialiasing](#antialiasing-2026-11-06).

## Checked

- 2026-10-30, `--native_x_renderer=true`, 2560x1080 window (ultrawide factor 1.33): the first screen with its side
  bars, the main menu, Quickplay, a song with the stage widened and the highway and HUD at 16:9; 584-761 draws a frame,
  none failed, 0 placeholder textures, GPU 1.3-1.9 ms a frame.

## Shadow quality: the filter follows the map

**Symptom** (Gustavo): on Ultra the shadows on the characters were better, still pixelated: a lattice of diamonds on skin.

**Cause:** GH5 reads its shadows in nine samples, each compared with the pixel's depth. `sub_825CCE98` sets their offsets to
one texel of the 400x400 map, 1/400. A map drawn four times as large (D92) kept them: the samples fell four texels apart.

**Fix:** `GH5ShadowFilterHook` (`0x825CD048`) divides the offsets by `native_x_renderer_shadow_scale`, so the samples are
neighbours on the larger map. Sharper edges, with a narrower soft edge. Nothing changes on the Xenos path or at Original.

**Off:** `gh5_shadow_filter_fix = false` in `gh5.toml`.

## The title screen blurred, the band see-through (2026-10-31)

**Symptom** (Gustavo): with NativeXRenderer the title screen was blurred all over and the band looked transparent,
the drum kit too, on the title screen and in songs; the Xenos path was sharp and solid. The earlier check had the
depth of field and the bloom turned off, which hid the blur.

**Causes:**
- The blur: GH5's depth of field and motion blur read a 320×180 depth the engine makes every other frame: the scene's
  1280×720 depth resolved through a 640×360 4x MSAA depth surface over the same EDRAM (one sample of each 2×2), drawn
  into a 320×180 depth surface and resolved again. NativeXRenderer read such aliased surfaces for color only (D95),
  so the 640×360 resolve read a target nothing drew into.
- The band: GH5 draws its scene over two frames. The first draws the stage and the crowd, resolves the color and the
  depth, and shows the last frame's image again. The second restores both and draws the band and the screen effects
  over them: it writes the depth resolve's words (read as k_8_8_8_8) through a color surface at the depth surface's
  EDRAM (base 720), with the halves of each EDRAM tile swapped as a depth surface stores them (the clear and the
  binding of the two targets at `0x8256A21C`, `0x82572350`). NativeXRenderer kept that color surface apart from the
  depth surface, so the band was drawn against a cleared depth.

**Fix:** NativeXRenderer D112 (the depth through an aliased MSAA surface too) and D113 (a depth target takes the
words a color target drew at its EDRAM; here the depth of the resolve those words came from, pixel for pixel). The
title screen and a Quickplay song look as on the Xenos path (the same camera shots compared: band, drum kit and
cymbals solid). Nothing to turn off.

## Antialiasing (2026-11-06)

**Symptom:** the launcher's antialiasing did nothing: the profile named no scene copy.

**Cause:** GH Van Halen's composite called Resolve itself, and its return address marked the scene. GH5 resolves
through the render targets' helpers (`sub_82571A30` -> `sub_82571288`), the path of 13 call sites, and draws its scene
over two frames:

| Frame | 1280x720 copies, in order |
|---|---|
| Stage and crowd | the scene without the band, from `0x82571B90`; shows the last frame's image with the 2D layer over it |
| Band and composite | **the whole scene, from `0x82571A64` (the first)**; the composite's first pass, the same callers and texture; the finished image, `0x8256A294 < 0x82570AE4`, read only by the next frame |

**Fix:** the first copy of the second kind, named by both callers. The size rule leaves out the 320x180 passes with
the same callers, and `scene_resolve_once` the composite's pass. It is before the bloom and the depth of field, so
they read the smoothed scene, and both frames show it: the next frame's image comes from it too. The finished image
would not do: it reaches the screen only every other frame.

**Checked:** one frame of RenderDoc, SMAA's input against its output from the same replay: 1.91 % of the pixels
change, on the edges, the mean brightness stays. Traces of the main menu, the band lobby, a song, the pause menu and
the first screen: the 2D layer and the note highway are always drawn after the copy, so they stay sharp. The same
with every screen effect off. Not smoothed: the lobby's character cards (drawn apart and placed with the 2D layer);
the song list has no 3D scene and no such copy.

**To turn it off:** the launcher's Antialiasing, or `native_x_renderer_antialiasing = "off"`.

## Stutters at 160 FPS (2026-10-30)

In a song no frame took over 15 ms at 3440×1440 / 160 FPS (the longest 7–9 ms). The longest frames were elsewhere:
- **A new screen's data.** When Party Play's song began, one frame took NativeXRenderer 56–61 ms: 37.6 MB of new
  textures copied from guest memory. Each page's first read cost 1.1 µs (the game wrote it through another view of
  the memory). NativeXRenderer D108 maps the guest memory ahead on a thread of its own and loads every texture
  through one scratch buffer: 38 ms (a 44 ms frame).
- **Pages watched at 160 FPS that 60 FPS would leave alone.** NativeXRenderer's D97 lets pages the game rewrites
  often be compared by hash instead of watched, by frame counts set at 60 FPS; GH5's ring buffer, rewritten every
  17–32 frames at 160 FPS, kept faulting. D109 scales them by the frame rate: the process 3.29 → 3.11 cores in a song.
- **The game's own loading** (40–90 ms frames of its main thread at screen changes) and NativeXRenderer's start (its
  draw path set up at the first frame, about 230 ms on a black screen) are left as they are.

D108 and D109 were tried in the other NativeXRenderer games (GH Metallica, GH Van Halen, GH Aerosmith, Kameo, GRAW,
POTCAWE): the same images, their frames' cost unchanged or lower, and shorter stutters while loading (GRAW 381 →
215 ms). Mapping ahead keeps 120–300 MB more of the system's memory (`native_x_renderer_map_guest_memory=false` turns
it off).

## The bloom's lattice (NativeXRenderer D127, 2026-11-07)

**Symptom:** in GH Metallica (a player's issue there, its `docs/bloom-lattice.md`), above 720 lines the glow around bright
lights doubled into offset copies or a grid of dots, and blurred backgrounds showed the same pattern. **Cause:** the
post-processing blurs reduced copies of the scene with tap offsets computed for their guest size; drawn at the render
scale, the taps skip texels. GH5 has the same kind of chain: the 640x360 4x MSAA resolves of the 1280x720 scene and of its
depth, 320x180 passes (color at EDRAM 1488, 1536, 1584 and 1709, depth at 1440) and a 160x90 one. **Fix:** the profile
sets `unscaled_screen_fractions` (`src/native_x_renderer_gh5.cpp`): those targets and their resolves stay at 1x, as on the
console, the 640x360 color copy averaging every pixel of the scene under it (the depth one takes one sample, D112). The
32x32-4x4 chain, 64x64, 128x128, 400x400 and the other sizes that are not fractions of 1280x720 stay at the render scale.
Checked: a run at 3440x1440 to the first screen (`gh5_300`): those targets and their resolves at 1x on the host, no warning.
**Off:** `native_x_renderer_unscaled_screen_fractions = false` in `gh5.toml`.
