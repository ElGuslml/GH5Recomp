# Stalls on a first play

**Symptom.** With NativeXRenderer, the first time a song was played the game could stop for up to most of a second in the
middle of it, 787 ms the longest noted, and the sound came late afterwards. Later plays of the same song did not.

**Cause.** A graphics pipeline the driver has never seen can take that long to compile. Most draws are skipped until
theirs is ready, one frame late at worst, but NativeXRenderer's D101 makes draws into offscreen targets wait for theirs, up
to 3 s: a skipped draw there can leave garbage in a texture the game renders only once, for the rest of the session. The
wait stopped the whole game, even for a pass the game draws again on the very next frame.

**Fix.** NativeXRenderer D126. A draw into a target the game has drawn into on more than 16 frames in a row no longer
waits: it is skipped until its pipeline is ready, and the pass mends itself. Every other offscreen draw still waits. The
stage's first frames, before a song, can still take that long, on purpose. In Party Play at 160 FPS with shaders new to
the driver, the song's 16 waits and its worst frame of 41 ms became no waits and 21 to 22 ms, and three whole songs played
that way had no stutter.

**How it was checked.** A window size not used before gives shaders new to the driver, so a first play can be repeated
without touching the driver's own cache. In GH Metallica's first song, where D101 was needed, the images stay clean, but
they stay clean with D101's wait off too, so they prove little by themselves: what makes the skip safe is that such a
target has already been drawn into, so it keeps its last frame and never shows undefined memory.

**Turn it off.** `native_x_renderer_skip_steady_passes = false` in `gh5.toml`.
