# Crash Bandicoot The Wrath of Cortex for Mac

Crash Bandicoot: The Wrath of Cortex (PS2) running natively on Mac, no emulator. The game's own code is recompiled from its PS2
executable to native code ahead of time, and a runtime stands in for the rest of the console, with the PS2's graphics chips
recreated on the GPU.

This is a work in progress and it still has bugs.

## Play

For Apple silicon Macs. Download `Crash-Wrath-of-Cortex-macOS.zip` from the
[releases](https://github.com/HAB3X/Crash-Wrath-of-Cortex-Mac/releases), unzip it and open Crash Bandicoot The Wrath of Cortex.
The first time, right click the app and choose Open, then pick your own USA disc image (SLUS 20238, v1.01). The game is not
included.

[docs/PLAYING.md](docs/PLAYING.md) has the controls and options.

## What works

* The game's code recompiled to run natively, at 60 fps
* Graphics on the GPU at up to 8x resolution, with texture filtering, texture upscale and anti-aliasing
* Sound and music: the game's own sound driver on a recreated PS2 sound chip
* Saving and loading, with no memory card screens or waits
* Keyboard, mouse and controllers, with remappable controls; click the game's menus with the mouse and type your name
* The game's own pause menu, with PC Settings in its Options (display, sound, controls and extras) in the Crash Twinsanity Mac
  port's style
* Cutscene skipping, faster loading and a level select
* The game's texts about memory cards and controller ports said as a computer's ([PC_TEXT.md](PC_TEXT.md))

## Still to do

* Intel Macs
* Playing every level from start to finish to check them
* Skipping the cutscenes inside levels (written, not yet tested)
* Linux and Windows

## Building

See [docs/BUILDING.md](docs/BUILDING.md).

## Credits

The [Crash Twinsanity Mac port](https://github.com/HAB3X/Crash-Twinsanity-Mac), whose graphics hardware layer, sound chip and
PC menus this shares. The Wrath of Cortex PS2 decompilation project, whose findings explained the game's code. The PS2 hardware
documentation and the work of the PCSX2 and PS2SDK projects.

Crash Bandicoot: The Wrath of Cortex is a game by Traveller's Tales and Universal Interactive. This repository has none of its
code or data: you need your own copy of the disc.
