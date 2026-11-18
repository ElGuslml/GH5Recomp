# The fixes and options from GH Warriors of Rock (2026-11-18)

**Status: built and checked with the keyboard, a virtual controller and screenshots (Party Play, 160 FPS, both
renderers: 0 errors); not played with a guitar yet.** GH Warriors of Rock (GHWoRRecomp) runs on GH5's engine. Its
work of 2026-11-14 to 18 was brought here without its online play, DLSS / FSR 3.1, the resolutions below 720 lines,
the ambient occlusion and the texture detail (those need the game's upscale point, left for later) and its crowd
switch. Each hook was found again in GH5's code through the two games' instruction indexes and the scripts' table of C
functions, then compared instruction by instruction; where GH5 differs, the section says how. The details of each
option are GH Warriors of Rock's notes (its `docs/`).

## The submodules

SDK `6e310b4` (changes 104-122), NativeXRenderer `c1b9b80` (D143-D180; D173 keeps the floor's shadows above the
Original shadow quality, the same surface here), RecompLauncher 3.31.1. NativeXRenderer's record thread is on by
default (`native_x_renderer_record_thread`, D154 / D155; D172 with V-Sync). **Off:** `native_x_renderer_record_thread =
false` in `gh5.toml`.

## Frame rates above 240

300, 360, 480 and Unlimited (1000 Hz) with V-Sync off, experimental; with V-Sync on a rate above the monitor's runs
at the monitor's (`src/options.cpp`). The presses kept for scripts held to 60 a second and the whammy's samples hold a
60th of a second at 1000 FPS (`src/framerate.cpp`). Party Play at 480: about 410 FPS.

## Star Power by Select above 60 FPS

GH WoR's pull request #5 (Panchovix). The script function `check_for_star_power_CFunc` (`sub_822390D8` here) reads
the one-frame pressed bits of Select (and of the D-pad on a controller) outside ControllerMake, so above 60 FPS a press
in a frame its held script skipped was lost. It is given those presses as ControllerMake is (`src/framerate.cpp`,
`GH5StarPowerPollHook` at its entry, the end hook at its two exits). Not tried with a guitar yet.

## The venue at the full frame rate

**Option, off by default:** Settings → Display → "Venue at the full frame rate", or `gh5_scene_every_frame = true`.
The engine draws its 3D scene over two frames (the split render mode), so the stage moves at half the frame rate.
`src/split_render.cpp` writes the mode 0 in both copies of the render state each frame (`0x82DA3728` and
`0x82D99918`, +3928 of GH5's render state; GH WoR's +1896). Checked: "the game's mode was 1", SMAA on every frame
(600 in 600), 0 failed draws. About twice the 3D work a frame (GH WoR's `docs/scene-every-frame.md`).

## D3D's fence stamps

`src/fence_stamps.cpp` (`gh5_fence_stamps_unwatched`, on): SetStreamSource's and SetIndices' fence stamps
(`0x8230F958`, `0x8230FA9C`) written through the physical view, so the renderers' page watches do not see them
(GH WoR's `docs/fence-stamps.md`). Party Play at 160: NativeXRenderer's buffer copies per draw about halved (0.45
against 0.87; different venues on screen). **Off:** `gh5_fence_stamps_unwatched = false`.

## Drum kits on five lanes

`src/drums.cpp` (`gh5_drum_kit_five_lanes`, on): the game gives five lanes only to a kit that holds the left stick's
click (a Guitar Hero kit's sign), so the keyboard, a Rock Band kit or a MIDI adapter played four whatever the
launcher said. A kit without the launcher's four-lane mark ("8:1") is now five-lane at its first read (`0x8248A0C8`;
GH5's controller port keeps the mark at +216 and the reads counter at +244). Checked with the keyboard as a drum kit:
"[drums] slot 0: a drum kit on five lanes". **Off:** `gh5_drum_kit_five_lanes = false`.

## A controller as in Guitar Hero III

The default **Play style** of a controller (`gamepad_guitar_layout = "gh3"`, SDK changes 112 and 113): in a song the
frets on LT, LB, RB, RT and A, the open note on B, no strum, Star Power on the D-pad or Back, the whammy bar on either
stick; its buttons in a song are its own (the launcher's "In a song"). GH5 kept Guitar Hero III's controller code
(`sub_821E12A8`, chosen when `sub_821D3450` says "not a guitar"); `src/controller_layout.cpp`'s 12 hooks make the
gameplay take such a controller for what it is, and three more let a chord over an extended sustain count (GH WoR's
`docs/controller-gh3-layout.md`). Checked with a virtual Xbox 360 controller: joined Party Play on guitar, LT+RB lit
green and yellow, A orange. **Off:** "Guitar · Xbox 360" in the controller's page, or `gamepad_guitar_layout =
"guitar"`.

## Xbox One guitars

SDK changes 110 and 114: an Xbox One guitar (PDP Riffmaster) is read through Microsoft GameInput's API 3, its tilt
included. The setup offers GameInput when the release ships `GameInputRedist.msi` (`Package-Release -WithGameInput`;
the build copies it when configured with `REXGLUE_GAMEINPUT_REDIST_MSI`), and so does the launcher's Controls tab
(GH WoR's `docs/xbox-one-guitars.md`). Not tried with such a guitar. **Off:** `gamepad_gameinput = false`.

## World Tour's Star Power sound

**Option, off by default:** Mods → Sound → "World Tour's Star Power sound", or `gh5_star_power_reverb = true`
(Panchovix's, GH WoR's pull request #8). GH5 keeps World Tour's echo, reverb and high pass structs but, as Warriors of
Rock, only raises the venue's reverb on Star Power; `src/star_power_reverb.cpp` builds World Tour's stack on the free
send slot 2 at the boot and sends the player's tracks there. GH5's tracks are 112 bytes with 24-byte sends, and its
send code also clears a byte through the same wrong slot as the volume it reads: the fix at `0x82168BF4` makes both the
slot's own. Checked: the stack made at the boot (5 of 64 effects taken). Not heard in a song yet.

## The mods: the note highway, two sounds and the speakers

Mods → Highway (`src/mods.cpp`, `src/script_globals.cpp`, `src/highway_images.cpp`, `src/highway_effects.cpp`): the
highway's scale with one player, its length, perspective, style (dark) and opacity, pictures from a gallery, and the
Star Power, band moment and revival effects at the highway's scale. GH5's highway values are GH WoR's to the digit;
the global table is at `0x82D784A4`. GH5's highway texture is `Highway_Judy_fm_01`, hash `5309C5000CB0C026` (for
`textures\` and the gallery), in `global.pak.xen` (the launcher's preview reads it there, RecompLauncher 3.31.1).
Mods → Sound: the whammy's bend (`0x821E9008`) and the instrument's volume on a miss (`sub_82162F20`; not in the
single-stream songs, which take another path). Settings → Audio → Channels with 5.1: the game is told its speakers
(`audio_speaker_config_from_output`), and `gh5_dolby_timing` keeps the Dolby Digital timing (-30 ms) whatever it is
told. Checked in Party Play: scale 85, length 150, perspective 75, dark, opacity 60 on screen; the texture log's hash;
the lag settings patched. Not checked: a miss's volume, the whammy's sound, the effects at another scale, a picture in
a song.

**Not ported, the crowd switch:** GH WoR hides its crowd with its own soundcheck cheat, which GH5 does not have. Read
`disable_crowd` as 1 in `create_crowd_models` (practice mode's state) and the venue's billboards show their
placeholder atlas (coloured rectangles). Hiding each ped (`hide_crowd_models`) is left to try (`src/mods.cpp`).
