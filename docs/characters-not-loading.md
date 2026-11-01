# Characters that never loaded after switching

**Fixed (2026-11-01), in the SDK (change 65).**

**Symptom:** in Quickplay's band lobby, moving through the characters quickly ("Personaje", "Elige un rockero") left
the player's card with a pocket-watch icon for good; no character loaded again, in the rocker creator either.

**Cause:** when the selection moves, the game cancels the model load in flight (`sub_82581C50`) and checks the result of
the kernel's `NtCancelIoFile`. The SDK had it as a stub that returned nothing, so the game read the file handle as the
status, a failure: it left the load request in its error state and never loaded a character again.

**Fix:** the SDK's `NtCancelIoFile` succeeds, as the console's does (the SDK's file reads complete at once, so nothing
is ever pending to cancel). Checked: switching characters quickly, 21 cancels, every character loads. Nothing to turn
off.
