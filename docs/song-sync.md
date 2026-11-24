# The notes and the music in step

**Status: built (2026-11-24); checked in Party Play, not played yet.** At 480 Hz with V-Sync off the game's own sums
would have put the notes +62 ms and +85 ms ahead in two songs of 4-5 minutes; at 160 FPS with V-Sync, +0.03 ms (nothing
to fix). Each song started with the clock 1-4 ms from the music's start.

GH Warriors of Rock's two fixes (its `docs/song-clock.md` and `docs/song-sync.md`, measured there with a probe that
compares the notes' clock with the music's position), on GH5's code: the same engine, the same functions, and here a second song clock that GH Warriors of Rock dropped.

## The notes drifting from the music during a song

**Symptom (GH Warriors of Rock).** With V-Sync off the notes gained on the music over the whole song (+28 to +73 ms in
three minutes); at each hitch of the game's main thread they stepped 3 to 10 ms either way; after a late audio device
callback the music stayed later for good. With V-Sync on and no hitch, nothing.

**Causes and fixes.**
1. **The SDK's guest clock gained on the host's** (SDK change 123, every game): two threads reading the time base at
   once could count the time between their reads twice.
2. **The game's own sum of the vertical blanks** (`src/song_clock.cpp`, `gh5_song_clock_exact`, on). Each frame the
   song's clocks add the intervals of the blanks that passed, from a ring of the last 10: the sum starts at the ring's
   oldest entry, and walks the ring again when a frame lasts more than 10 blanks. With uneven blanks (V-Sync off) the
   sums no longer add up to the time that passed. The clocks now add the time between the blanks themselves, kept by a
   hook in the vblank callback. GH5 advances two clocks this way (`sub_823EB788` and `sub_823EB8D8`; part of the
   gameplay reads the second), and both are fixed.
3. **A gap in the audio device's output** (SDK change 124, `audio_keep_time_device_gaps`, on): when the device runs
   dry its lost frames are dropped from the game's sound, so the music catches up.

## The music starting late after a song that follows another

**Symptom (GH Warriors of Rock).** In a song that follows another without the menus, with an audio calibration more than
50 ms above the video's, the notes stayed ahead of the music until a pause (172 ms with 0 / 120). GH5's script that
starts a song (`start_gem_scroller`) does the same arithmetic word for word: the clock starts at 0 less the
transition's time less twice the calibration's lag, so a short transition starts it past the music's start.

**Fix** (`src/song_sync.cpp`, `gh5_song_start_sync`, on). While a song's stream is loaded and waits to play, a reset of
the clock past its start is moved 100 ms before it, and at the play a clock more than 30 ms past the start is set back
to it. Above 60 FPS the script that starts the music (`begin_song_after_intro`) runs every frame, so the music starts
within a frame of its time.

## In the log

At each song's start: `[song sync] the music starts from 0.000 s with the notes' clock -3.1 ms from it`, and what the
rules did. At its end: `[song sync] the song's clock: ... the game's own sums +0.01 ms from the blanks' times`, for
each clock (with `gh5_song_clock_exact` off, that figure is the clock's error in the song).

## Turning it off

`gh5_song_clock_exact = false`, `gh5_song_start_sync = false` and `audio_keep_time_device_gaps = false` in `gh5.toml`.
The SDK's clock fix has no switch.
