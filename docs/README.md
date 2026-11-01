# Documentation

Short notes on each problem found while making the game run, and what was done about it. Each one gives
the symptom, the cause, the fix and how to turn the fix off, so that you can check it on your own machine.

Guitar Hero: Van Halen was built on Guitar Hero 5's engine, so GH5 has its problems. GHVanHRecomp's fixes (most of
them GH Metallica's, ported there) were ported with every address translated; the investigations are GHMetalRecomp's
and GHVanHRecomp's.

| Note | Status |
|---|---|
| [The fixes ported from GH Van Halen](fixes-from-gh-van-halen.md) | In testing: the threads' waits, the song clock, the audio, the guitar, the keyboard and a controller as a guitar, frame rates above 60, ultrawide, the screen effects' switches (GH5's own composite), quitting from the main menu; the crowd's claps not ported (GH5's scripts look fixed); the profile following the press not active (GH5 starts in Party Play) |
| [NativeXRenderer in Guitar Hero 5](native-x-renderer-gh5.md) | Working: GH Van Halen's map and profile, translated; no antialiasing yet (the scene's copy not told apart) |
| [Characters that never loaded after switching](characters-not-loading.md) | Fixed in the SDK: its `NtCancelIoFile` succeeds |
