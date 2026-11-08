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

`signin_ui_cancel = true` and `signin_guests = true`, as GH Van Halen's. A guitar that pressed Start with a controller
connected before it was the profile's guest: GH Van Halen's profile following the press at the title screen was off
here, because GH5 has no title screen. It starts in Party Play (songs playing while the players join), which is told
apart now by a screen element (0x22F50BBE, `src/quit_prompt.cpp`). The first controller to press a button there takes
the profile and the others are its guests. Party Play stays after that press, so it counts once, for good: a second
player joining the song, or Party Play entered again from the main menu, moves nothing (`src/players.cpp`). Tried with a
DualSense and a guitar connected.
**Off:** `gh5_profile_follows_press = false`.

## GH Metallica's of 2026-11-03/05

Moved on 2026-11-05 to GH Metallica's versions: SDK `0a7c86c` (changes 69-72, on its `ghmetal` branch), NativeXRenderer
`6117e82` (D118-D125), RecompLauncher `1ee6325` (3.13.1). The game's side, with this game's addresses:

- **V-Sync with G-Sync or FreeSync** (GH Metallica's `docs/vsync.md`): the SDK's timer, for a display whose vertical
  blanks follow the game's frames, ticks within microseconds (`vblank_timer_spin_us = 700`, SDK change 70), 0.25 % under
  the display's exact rate; the render thread wakes on events the game's swap and vertical blank callbacks set
  (`src/thread_waits.cpp`, hooks at `0x823B042C` in `sub_823B03F0` and `0x823B03D8` in `sub_823B0378`). With V-Sync off
  the game runs 0.25 % under the display's exact rate when the frame rate chosen is that rate (`src/options.cpp`).
  **Off:** `vblank_timer_spin_us = 0`.
- **Alt+Tab from fullscreen** no longer closes the game (NativeXRenderer D125).
- **Each slot's kind of controller follows the one played**: the game asks a port for its kind only when its controller
  connects, and the keyboard shares slot 0 with the first controller. When the SDK finds the slot played on another
  kind, a hook at the entry of the port's update (`sub_8248A4A8`) has the game ask again, outside a song
  (`src/players.cpp`). Not tried with a second controller here. **Off:** `gh5_kind_follows_controller = false`.
- **The keyboard's controls**: Shift, Ctrl and Alt as keys of their own, F1-F12 too; the SDK's developer keys are off
  (`debug_hotkeys = false`; the launcher's Settings > Game > Debug keys turns them on).
- **Guitars**: `guitar_ps3_protocol = true` (a PS3 Guitar Hero guitar, a CRKD guitar on its P3 setting); the launcher
  keeps every controller open and assigns any axis to the whammy bar and the tilt (SDK changes 71 and 72, launcher 3.12
  and 3.13). Not tried with those guitars here.
- **The guitars' strings above 60 FPS**: the script that places them is not held to 60 passes a second (`0x02B91F04`,
  `BandManager_PrepareBandMemberForRender` then `Wait 1 gameframe`): held, they stayed where the guitar had been in
  between and shook around a moving guitar. Placed every frame, their vibration (62 Hz, which 60 FPS shows as a 2 Hz
  wobble) is kept to that wobble and its fade to 0.99 per 60th of a second (hooks at `0x821BA658` in `sub_821BA5D8` and
  `0x821B3904` in `sub_821B38C8`).
- **The audio's spacing stays**: GH Metallica and GH Van Halen went to `audio_callback_min_interval_us = 0` because FMOD
  mixes on their audio worker. Here FMOD mixes on its own thread, which needs the 3 ms (their own note says so): 0 would
  bring the noise back. SDK change 69 (`audio_keep_time` never drops a frame the device is about to take) comes with the
  move.
- **Not ported**: GH Metallica's Havok step. `sub_82634770` steps the world by a fixed 1/60 s here too, but stepped by
  the frame's own time above 60 (a hook at `0x8263479C`, tried on 2026-11-05) the hair and coats stretched in long
  spikes at 160 FPS: this game also keeps the step over the frame's time (+80 of the object). It stays as it was, as it
  has been played: the world runs at 160 / 60 of its speed at 160 FPS.

## Frame rates above 60

**Experimental.** `gh5_frame_rate` (60, 120, 144, 160, 165, 240; the launcher's frame rate row) sets the vertical
blank's rate; the game's clock is real time. `src/framerate.cpp` (`gh5_frame_rate_fix`, on) keeps at the console's
speed what counts frames: the vblank clocks (`sub_82637918`), the scripts' `Wait N gameframes` (`sub_8257E498`,
`sub_8257E448`, `sub_8257E798`; also GH5's script function Yield, which waits a game frame), the button presses
scripts poll (`sub_824B58C8`, ControllerMake `sub_824C06B0`), the whammy on a sustain (`sub_821E6DB0`,
`sub_821E1A38`, `sub_821E96C8`) and the loading screen's animations (`sub_825E1320`, `sub_82427348`, `sub_825E1698`).
Star power's drain (script `0x74B936AB`, the same in GH5) is left to run every frame. GH Van Halen's frame time hook
is not needed: GH5 measures the frame's time on the time base. **Off:** 60 in the launcher. Played at 160 FPS with a guitar (2026-11-06).

**When the frame rate falls below the chosen one (2026-11-07).** Some of what the fix counts in frames took a frame as
one vertical blank at the chosen rate: the whammy's 5 frames, the strings' fade (0.99 a frame), the loading screen's
steps, and the margins that pick the frame a script wait ends in and the presses given to scripts. When the game could
not keep that rate (a frame that takes 5 ms at 240 Hz is shown for 8.3), those ran slower until it came back. They use
the frames' real length now, measured once a frame on the main thread (`GH5MainFrameHook` at `0x82413BCC`, `sub_82413AA0`):
the last frame's for the strings' fade, the average of about the last ten for the rest; the loading screen's animations,
drawn by the render thread while the main thread loads, measure their own. The whammy's count of frames in a row is
compared with 5 for equality, so each run keeps the number of frames it began with. The notes on the highway are not
among them: they follow the game's own clock. **Off:** 60 in the launcher.

**The frets falling back (2026-11-05).** On a hit the fret heads rise 20 pixels and fall back in 0.1 s; at 160 FPS the
fall took 0.27 s. Each pass of the script `button_checker` (`0x716BADDC`, through `sub_821E8500`) lowers them by the
frame's time, and the fix above held that script to 60 passes a second. It now runs every frame, as star power's drain:
100 ms at 160 FPS as at 60 (measured in a Quickplay song).

**Self-timed scripts (2026-11-06).** Five scripts that add the frame's time to their own timers each pass (`0xEC23DF7E`,
`0xE66D1DA1`, `0x4E850682`, `0xE20DF41E`, `0x3E4FDDAF`) ran their timers at 60 / 160 of the speed at 160 FPS, held to 60 passes a
second. They run every frame now (`kSelfTimedScripts`). Left out: the jam studio's arrows, `0xCFDDF140`, which also steps by the
pass: at 160 FPS its scroll stays slower than at 60.

**Camera cuts above 60 (2026-11-06).** At 160 FPS each camera cut showed, for a few frames, a strange image in the top left
of the screen, then the stage from a camera not yet placed; at 60 nothing (Gustavo, frame by frame in a video). GH5 draws
the stage and the crowd in one frame and the band in the next (`IsPrimaryFrame`, `sub_821B6658`), and script `CAD261DF`
waits 2 game frames in a primary frame and 1 in the other so that what follows lands in one kind of frame. The script waits
above turned those into time, 3 or 5 frames at 160 FPS in either kind. The image was the venue screens' 512x512 surface,
left in EDRAM by a frame with no scene. That script and the seven that go on from it in whole frames run every frame now
(`kFrameExactScripts`): the frames with no scene went from 18 runs in 8000 frames to none. Only GH5 and GH WoR have
`IsPrimaryFrame`. Nothing to turn off: at 60 FPS nothing changes.

**The drummer above 60 (2026-11-06).** At 160 FPS the drummer moved in slow motion and missed the drums; at 60 he kept the
song. His script (`0x473164C7`) counts down the time to each hit by its step, two frames' `GetFrameLength` (33.369 ms to begin
with) across a `Wait 1 frame`, and that wait is a timed one, 1/60 s. At 160 a step of 12.5 ms took 37.5: his timers ran at a
third of the speed. Above 60 Hz this script's waits are real frames now (`GH5DrummerWaitHook` at `0x824AB938`, `kDrummerScript`),
as GH WoR's script has them: 80 passes a second at 160 FPS (measured), so his arms update more often than on the console.
**Off:** 60 in the launcher.

## The crowd's claps on star power

**Not ported.** In GH Van Halen the claps came late by the audio calibration (`src/crowd.cpp` there). GH5's scripts
register the claps without the calibration term that made them late, so its claps are probably on time already; the
port would make them early. To be checked in a song with star power and a calibration.

## Ultrawide

The same as GH Van Halen's (`src/widescreen.cpp`, 8 hooks, each on the same instruction, and a ninth of GH5's own):
every 16:9 perspective is widened (Hor+, `sub_823D9828`), and the 2D layer is narrowed to 16:9 in the center; outside a
song black bars cover the sides. Checked with NativeXRenderer at 2560x1080 (factor 1.33): the bars at the first screen,
the stage filling the screen in a song with the highway and HUD in the center. GH5's red border while the rock meter
is red (with the screen effects' darkening under it) is drawn as 2D sprites, quarters of the 1280x720 canvas from each
corner to the middle: narrowed with the HUD, it was a red frame around the middle 16:9. In a song, a sprite reaching a
side and the top or bottom of the canvas and at least a quarter of it is left as it is, so the border covers the whole
width (2026-10-30, at 2560x1080 and 3440x1440). **Off:** 16:9 in the launcher (`gh5_ultrawide = false`);
`gh5_ultrawide_hud = false`; `gh5_menu_bars = false`; `gh5_ultrawide_overlays = false` (the border at 16:9 again).

**The black band beside the highway (2026-11-05).** In ultrawide a soft black band ran along both sides of the highway;
at 16:9 there was none. GH5 starts its scene with a depth only quad in the highway's shape at the near plane
(`sub_822FFA78`, vertices in pixels of the 1280x720 canvas), so the stage is not drawn under the board. It went through
none of the 2D layer's hooks, so it stayed as wide as the stretched canvas while the board over it was narrowed: the
stage failed its depth test beside the highway, leaving the clear's black, blurred by the screen effects.
`GH5HighwayMaskHook` narrows it with the rest (factor 1.34: NativeXRenderer and the Xenos path at 1720x720, a song and
Party Play's difficulty list). It follows `gh5_ultrawide_hud`, as the board does.

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

## The job queue's lock and wake-ups (2026-11-08, from GH Metallica)

GH Metallica's trace of a song at 160 FPS (its `docs/cpu-usage.md`): the main thread and the three job workers slept
and woke each other tens of thousands of times a second on the job queue's critical section, which the game creates
with no spin count, and each job queued woke all three waiting workers (a pulse of a manual-reset event) for one of them
to take it. The same engine here, so the same two fixes:
- `critical_section_spin_us = 20` (the SDK's change 75, set in `src/startup.cpp`): a thread finding a game lock taken
  watches it for up to 20 us before it sleeps. GH Metallica: 92,000 -> 31,300 switches a second, 1.84 -> 1.66 cores.
- `gh5_job_wake_one` (on; `src/thread_waits.cpp`, the job event's pulse hooked): one waiting worker is woken per
  job. GH Metallica: each worker 5,300 -> 3,400 wakes a second, 1.98 -> 1.84 cores.
Not measured here yet. Off: `critical_section_spin_us = 0`, `gh5_job_wake_one = false`.
