# Crash Bandicoot: The Wrath of Cortex for Mac

Crash Bandicoot: The Wrath of Cortex (PS2) running natively on Mac, no emulator. The game's executable is recompiled to C and
runs on the [PS2 native runtime](../ps2-native-runtime).

This is a very early work in progress and it still has bugs. It boots into the game's own code, loads its files, draws with
the game's own graphics programs, and plays from the title screen into the Warp Room: Crash moves, and the game saves to
and loads from the memory card (a folder on your Mac). Its texts talk about a computer, not a PlayStation 2 (`PC_TEXT.md`). The sound is the game's own: its sound driver's
commands play its samples and streams (music, voices) from the disc's SFX.DAT on the runtime's SPU2.

## Still to do

* Full speed, the levels
* The reverb (the game never turns it on so far)
* A Mac app

## Building

You need your own copy of the game: the USA release, v1.01 (SLUS-20238), as a .cue/.bin or an .iso. Clone this repository next
to ps2-native-runtime, then:

    cmake -S . -B build -G Ninja -DWOC_DISC="/path/to/your/game.cue"
    cmake --build build
    ./run.sh

`run.sh` takes the disc from `WOC_DISC` or from `local.json` (`{"disc": "/path/to/game.cue"}`). Controls (the Crash Twinsanity port's PC controls): W A S D move, Space jumps, the left mouse
button or J spins, Left Shift or K crouches and slides, Tab or I is triangle, Z and C the shoulders, Return pauses (the
game's own pause), Left Ctrl walks, the arrows are the d-pad; a gamepad works too. On the name screen just type your name
(Backspace deletes, Return is DONE). Escape (or a gamepad's Back button) opens the PC menus, as in the Crash Twinsanity port:
the game pauses under them; Options has the window or fullscreen (Cmd+F or Alt+Enter at any time), the picture's resolution
and filters, the volume and every key to rebind, and Quit leaves the game. The settings are kept in
`~/Library/Application Support/PS2 Native Runtime/SLUS_202.38/` (`bindings.txt`, `graphics.txt`, `options.txt`).
Saves go to
`~/Library/Application Support/PS2 Native Runtime/SLUS_202.38/memcards/port0`. `--headless --snapshots DIR
--snapshot-every 5` saves pictures instead of opening a window; `--mute` plays no sound, `--audio-dump FILE.wav` keeps what's
played, and `WOC_SOUND_TRACE=1` logs the sound driver's commands.

The picture is drawn on the GPU at 3 times the PS2's resolution. `--scale 1-8` changes that, `--texture-upscale 2` (or 4)
smooths the game's textures, and `--gs software` draws with the PS2's own rules at its resolution instead (slower); the same
settings are in `graphics.txt` next to `bindings.txt`.

The executable is read from your disc at build time and the recompiled code stays in `build/`.

## Credits

The Wrath of Cortex PS2 decompilation project, whose findings explained the game's code. Crash Bandicoot: The
Wrath of Cortex is a game by Traveller's Tales and Universal Interactive. This repository has none of its code or data: you
need your own copy of the game.
