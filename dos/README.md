# Tomb of the Assembly - DOS edition (x86 assembly)

The whole game is written in **x86 assembly** (16-bit real mode, 80386 instructions, GNU `as` Intel syntax)
and builds a 24 KB DOS program, `TOMB.COM`, that runs in DOSBox.

* **How to run it, step by step: see [HOW_TO_RUN.md](HOW_TO_RUN.md)** (double-click `RUN.BAT` after installing DOSBox).
* Same rules, controls and five levels as the ICMC version, but zoomed in (16x16 tiles, smooth scrolling camera),
  with a Tomb-of-the-Mask-style endless-mode generator, a rising lava chase in endless mode, a stage-failed death overlay, and an instant room change.

![screenshots](docs/screenshots.png)

## What changed compared to the ICMC version
| | |
|---|---|
| Platform | DOS / DOSBox, VGA 320x200, double-buffered, synced to the 70 Hz retrace |
| Zoom | about 20x10 tiles visible, camera follows the player; auto-tiled walls, sprites, spiked traps |
| Maps | endless mode builds tall mirrored tomb mazes with chunky 3x3 wall structures, loops, chambers and pillars instead of thin Pac-Man-like corridors. A solver places traps, coins and shields: every room can be finished, nobody gets stranded, every coin is on a real dash path |
| Endless pressure | a red lava trail climbs from below, advances every second, and ends the run when it reaches the player |
| Trap feedback | hitting an unshielded trap displays **STAGE FAILED** before restarting a normal stage or ending an endless run |
| Slow room change | the ICMC version redrew all 1200 cells 30-40 times and waited ~2.7M simulated instructions after each step. Here the change is one memory blit per frame (~0.3 s) and only changed frames are drawn. A patched `Simulador/tomb_of_mask.asm/.mif` (4 words changed) makes the old version ~4x faster too |

## Is it really assembly?
Yes: every file in `src/` is assembly (`.s` / `.inc`). The only generated parts are data tables
(sprites, font, palette, the levels copied from your `.asm`) written as `.byte` lines by `tools/*.py`.
The folder `reference_c/` holds a C implementation of the same game. It is **not** part of the program:
it is only used by the tests as an oracle.

## How it was tested
`make test` runs the assembly build and the C reference side by side in a small 386 emulator (`tests/x86run.c`)
and compares them:
* generated rooms (60+ random seeds, depths 1-60): byte-identical, including the random generator state;
* frames: identical pixel for pixel for the menu, all five levels played start to finish (including the
  room-change scrolls), random key presses through menus/settings/help/bombs/restarts, and four solver-driven endless rooms.

**Not tested here:** real DOSBox and a real Windows machine were not available. The emulator is my own and covers
the instructions this program uses. Please smoke-test it in DOSBox (see HOW_TO_RUN.md) and tell me if anything
looks different.

## Layout
    dos/   TOMB.COM  RUN.BAT  tomb.conf  HOW_TO_RUN.md  Makefile  src/  tools/  tests/  reference_c/
    Simulador/   patched tomb_of_mask.asm / .mif for the original simulator (faster room change)
