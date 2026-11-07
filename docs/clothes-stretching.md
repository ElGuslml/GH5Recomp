# Clothes shooting out for a frame

**Symptom.** Above 60 FPS a musician's clothes, most of all Axel Steel's long coat, shot out across the screen for a
single frame, in long wedges ending in a point, several times a song. Both renderers did it, at 110, 130 and 160 FPS.

**Cause.** An order in the game. GH5 skins its characters on the CPU: the three job workers compute each piece's vertices
from a palette of 128 matrices, and the draw reads the result. Updating a skeleton starts the animation jobs that write
that palette, in stages, then queues the skinning of its pieces (`sub_824767F0`, through `sub_82556348`), and only then
waits for those jobs. So the skinning could read a palette half written, with the bones of the clothes all still on one
matrix, and what hangs from them went with it.

Checked with a probe: the palette changed under the skinning 19 to 48 times a song, and one such moment fell on the very
second a stretch was noted down. The Havok bodies, the animation's pose and the memory the vertices live in were all
fine, and the skipped draws and the shaders' cache had nothing to do with it. It also happens at 60 FPS, fewer times: the
stretch was not seen there, and how often it would show is not known.

**Fix.** `src/skinning.cpp`: a hook at the start of `sub_824767F0` does that wait first, so the skinning starts on a
finished palette. The race went down to 1 to 3 times a song, one of them as the band is placed, and four songs with Axel
Steel at 160 FPS showed no stretch. The one frame where the whole band moves took about 30 ms and now takes about 44, in
Party Play.

**Turn it off.** `gh5_skinning_order_fix = false` in `gh5.toml`.
