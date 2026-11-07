# Documentation

Short notes on each problem found while making the game run, and what was done about it. Each one gives
the symptom, the cause, the fix and how to turn the fix off, so that you can check it on your own machine.

Guitar Hero: Van Halen was built on Guitar Hero 5's engine, so GH5 has its problems. GHVanHRecomp's fixes (most of
them GH Metallica's, ported there) were ported with every address translated; the investigations are GHMetalRecomp's
and GHVanHRecomp's.

| Note | Status |
|---|---|
| [The fixes ported from GH Van Halen](fixes-from-gh-van-halen.md) | Working: the threads' waits, the song clock, the audio, the guitar, the keyboard and a controller as a guitar, frame rates above 60, ultrawide, the screen effects' switches (GH5's own composite), quitting from the main menu, the first controller to press in Party Play takes the profile; the crowd's claps not ported (GH5's scripts look fixed); singing and drums not tried |
| [NativeXRenderer in Guitar Hero 5](native-x-renderer-gh5.md) | Working, the default renderer: GH Van Halen's map and profile, translated; SMAA and FXAA on the scene's copy |
| [Characters that never loaded after switching](characters-not-loading.md) | Fixed in the SDK: its `NtCancelIoFile` succeeds |
| [Clothes shooting out for a frame](clothes-stretching.md) | Fixed: the skinning waits for the animation that writes its bones |
| [Downloadable content](dlc.md) | Working: Guitar Hero 5's own packages add their songs; other games' install and add none |

Building the project, its configuration and how the SDK is changed: [`development.md`](development.md).
