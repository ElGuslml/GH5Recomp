# Downloadable content (DLC)

**Status: working (2026-11-06).** A package made for Guitar Hero 5, the Megadeth Track Pack, installs from GH5 Launcher's DLC
page and its songs show in the game.

GH5 Launcher's DLC page installs Xbox 360 content packages (STFS: LIVE, PIRS or CON) with the game itself:
`GH5Recomp.exe --install_content=<package>` extracts one into

    userdata\0000000000000000\41560840\00000002\<package name>\

## What was checked

With two packages at hand, neither made for this game (Guitar Hero II's Track Pack 2, World Tour's Guitar Duel Track
Pack):

- Both install, and the page lists them.
- The game finds and mounts what is installed: it asks for `CONT0:\bmanifest_e35cd47b.pak.xen`, GH5's manifest, which
  these packages do not have. With nothing installed it asks for nothing.
- Neither adds songs: Quickplay's list shows 92 with them and without them.

So a package for another Guitar Hero does nothing here.

The Megadeth Track Pack is GH5's own format (`bmanifest_*.pak.xen`, `bdlc1238_song.pak.xen`, three songs): with it the game's
request for a manifest no longer fails, and its songs are in the game.

## Turning it off

Remove the package from the DLC page, or delete its folder. `kHasDlcPage` in `launcher/game.h` hides the page.
