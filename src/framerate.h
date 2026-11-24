// gh5 - frame rates above 60, and the count of a song's frames by the vertical blanks each took (src/framerate.cpp).
#pragma once

namespace gh5 {

// A song's music stopped (the hooks of SongStop, SongUnload and SongAudioPlayerDeInit, src/song_sync.cpp): its frames
// by the blanks they took, in the log.
void FrameBlanksSongEnded();

}  // namespace gh5
