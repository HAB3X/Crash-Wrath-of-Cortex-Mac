# Playing Crash Bandicoot: The Wrath of Cortex on a Mac

This is a native Mac port of Crash Bandicoot: The Wrath of Cortex, made by recompiling the PS2 game's code. It contains no game
data: you need your own copy of the game.

## What you need

- A Mac with Apple silicon (M1 or later) running a recent macOS.
- A disc image of **your own** USA copy of the game, version 1.01 (SLUS-20238), as a `.cue`/`.bin` or an `.iso`. Other
  versions (the first USA release, PAL, Japan) aren't supported yet: the app checks the disc and says so.

Nothing else: the app carries its own libraries (no Homebrew needed).

## First run

1. Unzip `Crash-Wrath-of-Cortex-macOS.zip` and move **Crash Bandicoot The Wrath of Cortex.app** wherever you like
   (Applications is fine).
2. The app is signed ad hoc, not by an Apple developer account. The first time you open it, macOS may say it can't check it:
   right-click (or Control-click) the app, choose **Open**, then **Open** again. You only have to do this once.
3. On the first launch the app asks for your disc image with a file dialog. Pick your `.cue` (or `.iso`). It remembers it; to
   pick another, delete `disc_image.txt` from the settings folder (below).

## Where things are

Everything the game keeps is in **~/Library/Application Support/Crash Bandicoot The Wrath of Cortex/** (in Finder: Go > Go to
Folder…, then paste that path):

| What | Where |
| --- | --- |
| Saves (the game's memory card) | `memcards/port0/` |
| The options | `options.txt`, `graphics.txt` |
| Your controls | `bindings.txt` |
| The disc image's path | `disc_image.txt` |

Saving works like a PC game: no memory card screens or waits. Save at the Warp Room's save screen, or from PC Settings > Extras >
Save game while you're in the Warp Room; load from the main menu.

## Controls

A controller (Xbox, PlayStation, Switch and most others) works out of the box, as on the PS2. Keyboard and mouse:

| Action | Keys |
| --- | --- |
| Move | W A S D |
| Turn the camera | Q / E |
| Jump | Space |
| Spin | Left mouse button (or J) |
| Crouch, slide | Left Shift (or K) |
| Triangle | Tab (or I) |
| Shoulders (L1 / R1, L2 / R2) | Z / C, 1 / 3 |
| D-pad | Arrow keys |
| Walk / run | Left Ctrl toggles walking |
| Pause | Esc (or Enter) |
| Skip a cutscene | Hold any button |
| The game's menus | Arrow keys or the mouse, Enter or a click to choose, Backspace to go back |
| Your name | Just type it; Backspace deletes, Enter is DONE |
| Fullscreen | Cmd+F (or Alt+Enter) |

Everything can be rebound in **PC Settings > Controls**; the button prompts follow what you last used.

## The pause menu and PC Settings

The pause menu is the game's own (Resume, Options, Quit). Its **Options** has **PC Settings** in place of the TV's Adjust Screen,
which opens the PC menus, in the Crash Twinsanity Mac port's style:

- **Gameplay:** pause in the background, skip cutscenes, faster loading, the language (it follows your Mac's preferred
  languages: English, French, German, Italian, Spanish or Dutch).
- **Display:** window size or fullscreen, the internal resolution (1x to 8x the PS2's), texture filtering, texture upscale
  (2x, 4x), anti-aliasing (FXAA, or supersampling 2x and 4x, which is slower), the CRT filter, v-sync, and the levels'
  distance blur (off: the game draws it at the PS2's own resolution, so it looks much softer than the rest).
- **Sound:** the master volume, mute in the background, and the game's music and sound effects volumes.
- **Controls:** the button prompts' style and every key to rebind.
- **Extras:** a level select (every warp room's levels and the secret levels: nothing is unlocked or written to your save), and
  Save game in the Warp Room.

Going back from PC Settings goes back to the game's Options. If the game runs slowly, lower the internal resolution: 3x is the
default.

## If something goes wrong

The app writes what it's doing to its standard output: run it from Terminal to see it,
`"/Applications/Crash Bandicoot The Wrath of Cortex.app/Contents/MacOS/crash-woc"`. A crash prints a report there; please
include it with any bug report.

## Licences

Crash Bandicoot: The Wrath of Cortex and its content belong to their owners. The app bundles SDL 3 (zlib licence), the fonts
Luckiest Guy (Apache 2.0) and Fredoka (SIL Open Font Licence), whose licences are in `Contents/Resources/licenses`, and
stb_truetype (public domain).
