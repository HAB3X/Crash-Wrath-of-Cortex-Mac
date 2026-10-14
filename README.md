# Crash Bandicoot: The Wrath of Cortex for Mac

Crash Bandicoot: The Wrath of Cortex (PS2) running natively on Mac, no emulator. The game's executable is recompiled to C and
runs on the [PS2 native runtime](../ps2-native-runtime).

This is a very early work in progress and it still has bugs. It boots into the game's own code, loads its files, draws with
the game's own graphics programs, and plays from the title screen into the Warp Room: Crash moves, and the game saves to
and loads from the memory card (a folder on your Mac). Its texts talk about a computer, not a PlayStation 2 (`PC_TEXT.md`). The sound is the game's own: its sound driver's
commands play its samples and streams (music, voices) from the disc's SFX.DAT on the runtime's SPU2.

## PC features

The same as the Crash Twinsanity Mac port's, where Wrath of Cortex has them. The game's own pause menu is kept (Escape or
Return): its OPTIONS have PC SETTINGS in place of the TV's ADJUST SCREEN, which opens the runtime's PC menus (`src/menus.cpp`):
Gameplay, Display, Sound, Controls and Extras, with the settings and actions below.

* Cutscenes: hold any button (a key, a mouse button, a gamepad's button) for half a second to skip the scene playing, with
  "Hold any button to skip" shown while one can be skipped: the logos and the flight to the title, the intro and the endings,
  the scenes in the Warp Room and in space between the warp rooms, a level's own cutscenes. Each ends the way the game's own
  skip (start) or the scene's own end ends it (`src/skip.cpp`). Options > Gameplay > Skip cutscenes turns it off.
* Extras: a level select, each warp room's levels and the secret levels. The game goes on from its pause and the level is
  started as the warp room's portals start one (and after it the game goes back to the Warp Room) (`src/levels.cpp`). Nothing
  is unlocked or written to the save.
* Saving: the game's own save, its four slots in one file in the save folder. Extras > Save game (in the Warp Room) opens the
  game's own save screen, as its save point does; the save is read without the memory card's wait (no CHECKING SAVE DATA),
  and no screen mentions a memory card or a console (`src/saving.cpp`, `PC_TEXT.md`).
* Faster loading (Options > Gameplay, on): a level's loading screen ends as soon as the level is loaded (`src/loading.cpp`).
* The language follows the Mac's preferred languages (English, French, German, Italian, Spanish or Dutch, English otherwise),
  or Options > Gameplay > Language; a loaded save brings its own, as on the PS2. Options > Sound has the game's music and
  sound effects volumes (`src/options.cpp`).

`WOC_MENU_TEST` tries the settings and actions without the menus, e.g. `WOC_MENU_TEST="12:run:Arctic Antics;20:set:Music
volume=5"` (game time in seconds; `src/menutest.cpp`).

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
button or J spins, Left Shift or K crouches and slides, Tab or I is triangle, Z and C the shoulders, Escape or Return pauses
(the game's own pause menu), Left Ctrl walks, the arrows are the d-pad; a gamepad works too. In the game's menus the mouse
points and clicks, Return chooses and Backspace goes back. On the name screen just type your name (Backspace deletes, Return
is DONE). The pause menu's OPTIONS > PC SETTINGS opens the PC menus, as the Crash Twinsanity port has them: the game stays
paused under them; they have the window or fullscreen (Cmd+F or Alt+Enter at any time), the picture's resolution and
filters, the volumes, every key to rebind and the level select. The settings are kept in
`~/Library/Application Support/Crash Bandicoot The Wrath of Cortex/` (`bindings.txt`, `graphics.txt`, `options.txt`), and the
saves in its `memcards/port0`. `--headless --snapshots DIR
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
