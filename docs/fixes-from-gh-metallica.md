# The fixes from GH Metallica of 2026-11-11 to 13

**Status: built, not tried in the game yet (2026-11-13).** GH Metallica 1.0.0-beta.1's game code and launcher, without
its online play: GH5 is on the same SDK (`4348a34`), NativeXRenderer (`c8e3e5c`) and launcher (3.21.1). GH5's engine
is not GH Metallica's, so each site was found again in GH5's own code and scripts (GH Van Halen's map,
`tools/port/map3.pkl`, then the instructions compared one by one; the scripts from the disc's `qb.pab.xen`, named by
`dbg.pak.xen`). Where GH5 differs, the section says how.

## The game's debug menu

**Option, off by default:** the launcher's Settings → Game → "Game debug menu", or `gh5_debug_menu = true`. The main
menu gets a "(debug menu)" item at its end and the pause menu a "Debug Menu" item (not in the tutorials): Neversoft's
debug menu (New Game Modes; In Game: Skip Into Song, Change Venue, Select Slomo, the lag tests, Character Select; HUD &
UI: Show Song Time, Show Measures, Rock Meter Debug; Band Setup; Tools: Play Credits, Model Viewer, Toggle GPU Time,
Toggle CPU Time; Unlock All). Its texts are in English, some items need the developers' tools, and Unlock All changes
the save. **How:** the game adds these items when the script global `enable_button_cheats` is 1; its startup sets it to
0 on a retail build (`if RetailBuild`, GH Metallica's `if cd`). `src/debug_menu.cpp` makes it read as 1 only in the
two scripts that build those menus (`create_main_menu_elements`, `ui_create_pausemenu`); everywhere else it stays 0
(`guitar_startup_systems` would otherwise start the PC viewer). Hooks: the script interpreter's two expression
evaluations in `sub_823B3220` (0x823B33EC / 0x823B33F4, 0x823B3754 / 0x823B3758) and the `$name` resolve at the entry of
`sub_823AAB98`; the running script's name is at +244 of the interpreter's script (GH Metallica's +216). The log says
"[debug menu] script ... offers the debug menu" for each menu.

## Ending the intro video once the game has loaded

**Option, off by default:** Settings → Game → "Skip the intro video", or `gh5_skip_intro = true`. GH5's start is not
GH Metallica's chain of videos and legal notice: it plays one video, `Intro_FINALanimatic_0209` (23.4 s, the logos and
the game's intro), as its loading screen, loads the game behind it, then waits for the video's end; once loaded, Start
or the green fret ends it (`LoadingEndMovie` sets `end_movie_early`). The video cannot be refused as GH Metallica's are
(the loading screen waits for it to play before the game loads on), so `src/intro.cpp` makes `end_movie_early` read as
1 in `destroy_loading_screen_spawned`: the video ends the moment the game has loaded, as if Start were pressed then, and
Party Play follows. What plays while the game loads still shows. The same hooks as the debug menu (one hook per
address). The log says "[intro] the game has loaded: the intro video ends".

## Drums, five or four lanes

GH Metallica's of 2026-11-11 (SDK change 87, launcher 3.19.1; its `docs/drums.md`): Controls → a controller or the
keyboard → Instrument: Drums, then Drum kit: 5 lanes (Guitar Hero) or 4 lanes (the game's Rock Band highway, as with a
Rock Band drum kit on the console). GH5 has the same code: `sub_82489958` makes a drum kit (XInput subtype 8) the game's
type 11 and keeps its capability flags' bit 0, `sub_82489C60` turns 11 into 10 (five lanes) at its first state read
unless that bit was set, and the script function `UseFourLaneHighway` (`sub_821D3FB8` / `sub_821D3EC0`) plays type 11
on GH5's four-lane highway (its four-lane positions, gems and glow in the scripts). So launcher 3.21.1's
`kHasFourLaneDrums` stays true here. The keyboard's drum keys are its own (D F J K L, four lanes K green; Space the
kick, Backspace Star Power; `launcher/game.h`). Not played through a song yet. **Off:** Instrument: Guitar.

## Other things that came with the move

- The mouse cursor hides over the game after 2 seconds without moving it (SDK change 103): `docs/mouse-cursor.md`.
- Settings → Audio (launcher 3.19, SDK change 85: the output device by name, the channels, the volume, mute while
  another window is in front; low latency audio moved there from Game) and Settings → Graphics → the graphics card
  (SDK change 86, NativeXRenderer D141). Nothing of the game's own.
- NativeXRenderer D142 with SDK change 88 (on by default; GH Metallica's `docs/crowd-stutters.md`): a signal the game
  waits for waits only for the renderer's work submitted before it, not for everything queued since (in GH Metallica
  up to 2.6 ms a frame in a crowded venue). **Off:** `native_x_renderer_precise_gate = false`.

## Not ported

- **Online play** (GH Metallica's experimental online modes through Steam): not for GH5 now. The SDK's `online_play`
  is off by default and the launcher shows no Online category (`kHasOnline` false).
- **The tap notes' trail at ultrawide** (GH Metallica's `GHMetallicaTapTrailHook`): GH5 draws none. Its trail renderer
  (`sub_82302288`, GH Van Halen's `sub_821C82E8`) reads its control points from arrays at 0x82CCA5F8 / 0x82CCA6C0 that
  no code writes, and no script or code creates the trail (`createguitartaptrail` is never called): GH5's tap notes
  have no trail.
- **The red border over the whole width at ultrawide**: GH5's own fix, already here (`gh5_ultrawide_overlays`,
  `docs/fixes-from-gh-van-halen.md`).
