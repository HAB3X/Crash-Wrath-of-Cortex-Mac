# Building Crash Bandicoot: The Wrath of Cortex for Mac

The game's executable (USA v1.01, SLUS-20238) is statically recompiled: every function of the R5900 code is turned into C ahead of
time and built into a native Mac program. The runtime in `ps2-native-runtime/` stands in for the rest of the console: the kernel,
DMA, the IOP's modules (files, the CD, pads, memory cards), the SPU2 sound chip, and the GS, VIF and VU graphics hardware (drawn
on the GPU with OpenGL). This repository's own code is what's specific to Wrath of Cortex: its IOP sound driver, the PC
features, the texts and the app.

The repository has none of the game's code or data. You need your own copy of the game: building reads its executable from your
disc image, and the recompiled C stays in `build/` on your machine.

**Playing on a Mac:** download `Crash-Wrath-of-Cortex-macOS.zip` from the
[releases](https://github.com/HAB3X/Crash-Wrath-of-Cortex-Mac/releases) and read [PLAYING.md](PLAYING.md).

[ps2-native-runtime/DESIGN.md](../ps2-native-runtime/DESIGN.md) explains how the runtime is put together.

## What you need

- **A Mac with Apple silicon** and Xcode's command line tools (`xcode-select --install`).
- **The game**: a disc image of the USA release, version 1.01 (SLUS-20238), as a `.cue`/`.bin` or an `.iso`.
- **CMake** 3.20 or later, **Ninja** and **SDL3**: `brew install cmake ninja sdl3`.
- **Python 3.11 or later**, for the recompiler (it only needs Python's own library). The app's icon, made from the game's own
  save icon on your disc, also needs Pillow and NumPy (`python3 -m pip install pillow numpy`); without them the app is built
  with no icon.
- **Git**, to clone this repository: `git clone https://github.com/HAB3X/Crash-Wrath-of-Cortex-Mac.git`

## Building

In the repository's folder:

```sh
cmake -S . -B build -G Ninja -DWOC_DISC="/path/to/your/game.cue"
cmake --build build
./run.sh
```

The first build recompiles the executable (a few minutes); the build is `build/crash-woc.app`. `run.sh` runs it with the disc
from `WOC_DISC` or from `local.json` (`{"disc": "/path/to/game.cue"}`), passing its options on.

The release, the app made standalone (its libraries inside, no Homebrew needed) and zipped:

```sh
cmake --build build --target release
```

That makes `build/dist/Crash Bandicoot The Wrath of Cortex.app` and `build/dist/Crash-Wrath-of-Cortex-macOS.zip`.

## Options for testing

| Option | What |
|---|---|
| `--headless --snapshots DIR --snapshot-every S` (or `--snapshot-at S,S,...`) | Runs with no window, saving pictures |
| `--run-seconds S` | Stops after S seconds |
| `--press S:BUTTON,...`, `--type S:TEXT`, `--click S:X,Y`, `--ui S:KEY,...` | Presses buttons, types, clicks and uses the PC menus at those times of the game's own clock |
| `--settings DIR`, `--memcards DIR` | Another settings folder or save folder |
| `--fps` | Logs the frame rate |
| `--gs hardware\|software`, `--scale 1-8`, `--texture-upscale 0\|2\|4` | The renderer, its internal resolution and texture upscale |
| `--mute`, `--audio-dump FILE.wav` | No sound, or what's played kept in a file |
| `--log kernel,syscall,io,dma,sif,iop,gs,...` | Logs the runtime's parts |

`WOC_SOUND_TRACE=1` logs the sound driver's commands, `WOC_MENU_TRACE=1` the game's menus as they change, and `WOC_MENU_TEST`
tries the PC settings and actions without the menus, e.g. `WOC_MENU_TEST="12:run:Arctic Antics;20:set:Music volume=5"`.

## Layout

| Path | What |
|---|---|
| `src/` | Wrath of Cortex's own code: the sound driver (`nusound.cpp`), the PC features (`skip.cpp`, `levels.cpp`, `saving.cpp`, `loading.cpp`, `options.cpp`), the mouse in the menus and the name entry (`menus.cpp`, `typing.cpp`), the texts (`pctext.cpp`) |
| `config/woc.toml` | The recompiler's settings: the game's functions replaced by `src/` |
| `tools/` | The app's icon and the standalone app |
| `PC_TEXT.md` | Every text said as a computer's, with its original |
| `ps2-native-runtime/recomp/` | The recompiler (Python) |
| `ps2-native-runtime/runtime/` | The runtime: memory, the kernel, DMA, the IOP, the SPU2, input, the window and the PC menus |
| `ps2-native-runtime/hw/` | The GS, GIF, VIF and VU hardware, shared with the Crash Twinsanity Mac port |

`build/` is made by the build and isn't committed.

## Credits

- The [Crash Twinsanity Mac port](https://github.com/HAB3X/Crash-Twinsanity-Mac), whose graphics hardware layer, sound chip and
  PC menus the runtime shares.
- The Wrath of Cortex PS2 decompilation project, whose findings explained the game's code.
- The PS2 hardware documentation and the work of the PCSX2 and PS2SDK projects.
- Crash Bandicoot: The Wrath of Cortex is a game by Traveller's Tales and Universal Interactive. This repository has none of its
  code or data: you need your own copy of the disc.
