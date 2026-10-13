# Crash Bandicoot: The Wrath of Cortex for Mac

Crash Bandicoot: The Wrath of Cortex (PS2) running natively on Mac, no emulator. The game's executable is recompiled to C and
runs on the [PS2 native runtime](../ps2-native-runtime).

This is a very early work in progress and it still has bugs. It boots into the game's own code, loads its files, draws with
the game's own graphics programs, and plays from the title screen into the Warp Room: Crash moves, and the game saves to
and loads from the memory card (a folder on your Mac). It runs slower than the PS2 in places, and there's no sound yet.

## Still to do

* Full speed, the levels
* Sound
* A Mac app

## Building

You need your own copy of the game: the USA release, v1.01 (SLUS-20238), as a .cue/.bin or an .iso. Clone this repository next
to ps2-native-runtime, then:

    cmake -S . -B build -G Ninja -DWOC_DISC="/path/to/your/game.cue"
    cmake --build build
    ./run.sh

`run.sh` takes the disc from `WOC_DISC` or from `local.json` (`{"disc": "/path/to/game.cue"}`). Controls: arrows, W A S D,
X (cross), C (circle), Z (square), V (triangle), Return (start), or a gamepad. Saves go to
`~/Library/Application Support/PS2 Native Runtime/SLUS_202.38/memcards/port0`. `--headless --snapshots DIR
--snapshot-every 5` saves pictures instead of opening a window.

The executable is read from your disc at build time and the recompiled code stays in `build/`.

## Credits

The Wrath of Cortex PS2 decompilation project, whose findings explained the game's code. Crash Bandicoot: The
Wrath of Cortex is a game by Traveller's Tales and Universal Interactive. This repository has none of its code or data: you
need your own copy of the game.
