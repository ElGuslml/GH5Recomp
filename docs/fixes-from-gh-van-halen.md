# The fixes ported from GH Van Halen

**Status: in testing (2026-10-30).** Guitar Hero: Van Halen was built on Guitar Hero 5's engine, and GHVanHRecomp's
fixes (most of them GH Metallica's, ported there) are this game's too: GH5's first boot had the same problems (5.78
cores busy at the first screen, among others). They were copied with their names changed and every address
translated. GH5 was built with a newer compiler (XDK 2.0.8276; GH Van Halen 2.0.7978), so its code differs in
registers, scheduling, structure offsets and data: each hooked function was found with a function matcher over both
executables' instruction indexes and checked instruction by instruction. Where GH5's code differs, the sections below
say how.

Checked here (2026-10-30, by the tools, not yet by a player): the first screen, the main menu, Quickplay and a song
on both renderers (the sound not heard), the CPU, the timer fix, the quit prompt, ultrawide at 21:9, two screen
effects turned off.

## The threads' waits (CPU)

**Symptom:** 5.78 cores busy at the first screen. **Cause:** the game's threads wait by checking a condition again and
again, which costs nothing on the console: the job system's three workers (an empty queue, `sub_825E5BE0`), the main
thread at the start of each frame (`sub_82413AA0`, `sub_82411CC8`), the render thread waiting for the vertical blank
(`sub_8255ABE8`), the main thread waiting for a group of jobs (`sub_82567D20`; GH5 counts two numbers per group where GH
Van Halen counted one), and the D3D library's waits for the GPU (`sub_82310968`). **Fix:** `src/thread_waits.cpp`,
seven hooks: a wait spins briefly, then sleeps; the workers block on the job event the game pulses, the render thread
sleeps until shortly before the vertical blank it waits for. Measured: 1.62 cores at the first screen (Party Play,
the band playing in 3D), 1.67 in a song. **Off:** `gh5_wait_smart = false` (slices only), `gh5_wait_spin_us` /
`gh5_wait_sleep_us`.

The generated code keeps the registers in local variables (the codegen's `*_as_local` options): setjmp `sub_82910F90`
and longjmp `sub_82910C70`, used only by libjpeg and libpng, share their registers; non-volatile vector registers stay
in the context because this compiler passes values in them, as GH Van Halen's.

## The audio drifts out of sync during a song

**Cause:** the engine's timer replaces the timebase's 50,000,000 Hz by 49,875,000 (the old consoles' real timebase);
ReXGlue's timebase runs at 50 MHz, so the game's clock ran 0.25 % fast and the audio fell behind the notes. **Fix:**
`src/timer.cpp`, a hook in `sub_823B08F0` (0x823B098C) keeps the reported frequency. **Off:** `gh5_timer_fix = false`.
`gh5.toml` also sets `audio_maxqframes = 4`, the SDK's shortest output queue.

**After a stutter** (GH Metallica's `docs/song-stutters.md`): `audio_keep_time = true` and
`audio_thread_priority = true` (SDK changes 62 and 63). GH Van Halen's third part, `src/fmod_mix.cpp` (FMOD's polled
output `sub_8236C820` updated on the SDK's audio worker after each frame the render client `sub_82876C08` reads), is
**off** (`gh5_fmod_on_audio_thread = false`, 2026-10-31): on, the music stuttered and dragged. GH5's FMOD thread
updates once per block (188 a second, one block each), not every millisecond as GH Metallica's; moved to the worker,
the update mixed 13 blocks a second of 188 and the sound device replayed the stale two-block ring (53 % of the
submitted blocks were the one two before, in a dump of the title screen's output). Off, FMOD mixes all 188.

## Low latency audio

**Off by default**: GH5 Launcher's Settings → Game → Low latency audio (`audio_wasapi_exclusive`, SDK change 64)
takes the output device in exclusive mode, 10 ms kept in its buffer and 2 of the game's frames queued. Calibrate the
audio again after turning it on or off.

## V-Sync

**On by default**: GH5 Launcher's Settings → Graphics → V-Sync writes NativeXRenderer's
`native_x_renderer_present_mode` (`"vsync"`, FIFO) and the SDK's `vblank_follows_display` (change 61): the game's
vertical blank follows the monitor's. Calibrate the video again after changing it.

## Noise instead of music, and the songs' tracks

`audio_callback_min_interval_us = 3000` in `gh5.toml` (the audio output called at the console's pace; FMOD's mixer
polls a short ring) and the SDK's XMA decoder carrying each packet's skip count (always on), as in GH Van Halen.

## The guitar and the keyboard

As GH Van Halen's: `gamepad_subtype = 6` (a gamepad as a guitar), `guitar_whammy_rest` and `gamepad_whammy_rest`, the
keyboard as a guitar (frets D F J K L, strum the arrows, Star Power Space), the strum on each fret only in a song
(`src/keyboard.cpp`, a hook at `sub_821D4EF0`, a player's update in a song: GH5 runs it for the players who play),
each controller's own layout and the whammy bar's calibration (SDK changes 59 and 60).

## A second controller

`signin_ui_cancel = true` and `signin_guests = true`, as GH Van Halen's. GH Van Halen's profile following the
controller that presses at the title screen (`src/players.cpp`) is **not active yet**: GH5 has no title screen; it
starts in Party Play (songs playing while the players join), which is not told apart yet. The first controller
connected plays with the profile.

## Frame rates above 60

**Experimental.** `gh5_frame_rate` (60, 120, 144, 160, 165, 240; the launcher's frame rate row) sets the vertical
blank's rate; the game's clock is real time. `src/framerate.cpp` (`gh5_frame_rate_fix`, on) keeps at the console's
speed what counts frames: the vblank clocks (`sub_82637918`), the scripts' `Wait N gameframes` (`sub_8257E498`,
`sub_8257E448`, `sub_8257E798`; also GH5's script function Yield, which waits a game frame), the button presses
scripts poll (`sub_824B58C8`, ControllerMake `sub_824C06B0`), the whammy on a sustain (`sub_821E6DB0`,
`sub_821E1A38`, `sub_821E96C8`) and the loading screen's animations (`sub_825E1320`, `sub_82427348`, `sub_825E1698`).
Star power's drain (script `0x74B936AB`, the same in GH5) is left to run every frame. GH Van Halen's frame time hook
is not needed: GH5 measures the frame's time on the time base. **Off:** 60 in the launcher. Not tested above 60 here
yet.

## The crowd's claps on star power

**Not ported.** In GH Van Halen the claps came late by the audio calibration (`src/crowd.cpp` there). GH5's scripts
register the claps without the calibration term that made them late, so its claps are probably on time already; the
port would make them early. To be checked in a song with star power and a calibration.

## Ultrawide

The same as GH Van Halen's (`src/widescreen.cpp`, 8 hooks, each on the same instruction): every 16:9 perspective is
widened (Hor+, `sub_823D9828`), and the 2D layer is narrowed to 16:9 in the center; outside a song black bars cover the
sides. Checked with NativeXRenderer at 2560x1080 (factor 1.33): the bars at the first screen, the stage filling the
screen in a song with the highway and HUD in the center. GH5's red border while the rock meter is red (with the
screen effects' darkening under it) is drawn as 2D sprites, quarters of the 1280x720 canvas from each corner to the
middle: narrowed with the HUD, it was a red frame around the middle 16:9. In a song, a sprite reaching a side and the
top or bottom of the canvas and at least a quarter of it is left as it is, so the border covers the whole width
(2026-10-30, at 2560x1080 and 3440x1440). **Off:** 16:9 in the launcher (`gh5_ultrawide = false`);
`gh5_ultrawide_hud = false`; `gh5_menu_bars = false`; `gh5_ultrawide_overlays = false` (the border at 16:9 again).

## Turning off the screen effects

**Options, all on by default.** GH5's final composite (`sub_82643598`) differs from GH Van Halen's: two passes of the
shader `_Uber_ScreenFX`, chosen from a word of flags with one bit per effect (motion blur, bloom, god rays, depth of
field, tone mapping, color, grain, vignette, a visualizer), read three times (the passes' variants, then how each pass
draws). `src/screen_fx.cpp` takes the bits of the effects turned off out of each read: `gh5_fx_bloom`,
`_depth_of_field`, `_grain`, `_color`, `_vignette`. GH Van Halen's ink blotches are not part of GH5's composite: no
switch. With the color correction off the grain goes too (GH Van Halen's rule; GH5's grain is in the other pass, not
checked). Checked: bloom and depth of field off -> "effects turned off: flags 0x00A" in a song.

## Singing with a PC microphone

**Experimental, not tried in this game** (GH Metallica's): `usb_microphone_1` and the launcher's Controls tab, as GH
Van Halen's.

## Quitting from the main menu

B in the main menu asks whether to quit (A quits, B cancels): `src/quit_prompt.cpp`, a hook in the main thread's
frame (`sub_82413AA0`) looks up the main menu's element (the same as GH Van Halen's) in the element table at
`0x82DA78C4`; a dialog box over the menu (`dialog_box_container`) keeps it away. Checked: the main menu → B → the
dialog → B (cancelled); Quickplay's band screen → B goes back to the main menu with no dialog. Not offered in Party
Play, where B is a joined player's red fret.

## Language

The game picks its text pack from `XGetLanguage` (English, French, German, Italian, Spanish); the launcher's
language row sets it. Not checked in this game yet.
