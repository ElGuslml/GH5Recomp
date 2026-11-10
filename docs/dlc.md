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

## Many packages: "Disc Read Error" at start (fixed in 0.2.1-beta.1, SDK change 78)

In GH Warriors of Rock, a player's collection of 190 packages showed the "Disc Read Error" dialog a few seconds after
starting. The game lists its content one package at a time with the XDK's asynchronous `XEnumerate`, which ends each
call with `ObDereferenceObject` on the enumerator. The SDK looked that object up through a handle it keeps in a dispatch
header, which an enumerator does not have: after the first call it released whatever object had taken a freed handle,
sometimes a file the game had just opened, and the game's next read of it failed. Now the object is found by its guest
address.

GH5 has the same XDK code (`sub_8281C520`: `XMsgInProcessCall`, the 308-byte `XCONTENT_DATA` copied out,
`ObDereferenceObject`, `XMsgCompleteIORequest`). Here the 190 calls ran on one thread within a millisecond, about 26 s
after the start, so a stale release rarely meets another object: two runs with the same 190 packages before the fix
showed no error. The fix keeps it from happening at all. Details: GHMetalRecomp's
`local_docs/39-dlc-disc-read-error.md`.

## A package of another game: "A content package appears damaged or unreadable" (launcher 3.15.0)

A player installed packages of other games; the game then showed "A content package appears damaged or unreadable. You
will not be able to use or access any downloadable content." and no DLC worked, its own packs included. The game's DLC
script (`5A4B6A0C`) resolves every content folder by a manifest: it knows a package by the QBKey of its display name
(lower case, any other character than a letter or digit an underscore; `sub_823C0608`) and looks for
`bmanifest_<key>.pak.xen` in it. GH5's packs carry theirs (Megadeth's: `bmanifest_285458e5.pak.xen`, the key of
"megadeth_track_pack"), and each lists the packs before it; the disc's own list names only Death Magnetic. A folder it
cannot resolve makes it ask for that file (`[NtCreateFile] FAILED: path='CONT0:\bmanifest_e35cd47b.pak.xen'` for Guitar
Hero II's Track Pack 2), set the damaged flag, remove every content file and show the dialog. On a console a package of
another game never lands in GH5's content folder.

GH5 Launcher 3.15.0 checks each installed package the same way (`kDlcRule` in `launcher/game.h`) and turns off the ones
the game cannot resolve: their folders move to `userdata\0000000000000000\41560840\Disabled DLC`, where the game does
not look. Each package has a switch; one turned on by the player stays on. Checked: with Guitar Hero II's pack off the
game lists only Megadeth's and asks for no missing manifest; on, it asks for `bmanifest_e35cd47b.pak.xen` again.
Details: `local_docs/20-dlc-damaged-warning.md`.

## Turning it off

Turn the package off on the DLC page (or remove it), or move its folder out of `00000002`. `kHasDlcPage` in
`launcher/game.h` hides the page.
