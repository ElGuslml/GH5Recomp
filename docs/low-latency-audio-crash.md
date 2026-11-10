# Low latency audio closed the game at startup

**Fixed (2026-11-10), in the SDK (change 81).**

**Symptom:** with GH5 Launcher's Settings → Game → Low latency audio on, the game closed before its first screen and
the launcher said "The game closed with an error (code 0xC0000005)". Every player with the setting on, any device.
Guitar Hero Warriors of Rock and Guitar Hero: Metallica were fine.

**Cause:** GH5 registers two audio clients at startup: the game's sound and the voice engine's (`XHVCreateEngine`).
The SDK opened the output device in exclusive mode for each one. Windows lets one exclusive stream hold a device, so
the second open was refused (the game's own first stream held it), the shared output could not open it either, and
the registration failed. The game does not check that result and read through a null pointer.

**Fix:** the device is opened once, and every client's sound is mixed into it, as the console does. With one client
the sound is the same as before. Checked: the game starts with the setting on and plays Party Play's music, no
errors; it closes cleanly. To avoid it on an older build, turn Low latency audio off.
