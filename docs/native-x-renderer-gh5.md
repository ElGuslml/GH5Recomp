# NativeXRenderer in Guitar Hero 5

The game's side of NativeXRenderer (`native_x_renderer/`, the library that hooks a game's Direct3D calls and draws
on the host). The hooks and the profile are in [`src/native_x_renderer_gh5.cpp`](../src/native_x_renderer_gh5.cpp);
the library's checklist is `native_x_renderer/docs/integrating-a-game.md`.

**Status (2026-10-30):** GH Van Halen's side (GH Metallica's map), translated. Not the default renderer:
`--native_x_renderer=true` or the launcher's NativeXRenderer card.

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
- **`scene_resolve_caller = 0`: no antialiasing yet.** GH Van Halen's composite called Resolve itself; GH5's
  (`sub_82643598`, two passes) resolves through the render targets' helpers, the path of every resolve. A traced song
  frame (`--native_x_renderer_trace`, F9) has the scene's 1280x720 copy after the 640x360 and 320x180 passes from
  `0x82571324 < 0x82571B90`, but `sub_82571AB8` (the outer level) has 19 callers: not set until the menus are traced
  too.

## Checked

- 2026-10-30, `--native_x_renderer=true`, 2560x1080 window (ultrawide factor 1.33): the first screen with its side
  bars, the main menu, Quickplay, a song with the stage widened and the highway and HUD at 16:9; 584-761 draws a frame,
  none failed, 0 placeholder textures, GPU 1.3-1.9 ms a frame.

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
