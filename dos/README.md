# Tomb of the Assembly - DOS edition (x86 assembly)

The whole game is written in **x86 assembly** (16-bit real mode, 80386 instructions, GNU `as` Intel syntax)
and builds a 26 KB DOS program, `TOMB.COM`, that runs in DOSBox.

* **How to run it, step by step: see [HOW_TO_RUN.md](HOW_TO_RUN.md)** (double-click `RUN.BAT` after installing DOSBox).
* Same core rules and controls as the ICMC version, but zoomed in (16x16 tiles, smooth scrolling camera),
  with a five-map campaign, a Tomb-of-the-Mask-style endless-mode generator, rising lava in endless mode,
  a stage-failed death overlay, and an instant room change.

![screenshots](docs/screenshots.png)

## What changed compared to the ICMC version
| | |
|---|---|
| Platform | DOS / DOSBox, VGA 320x200, double-buffered, synced to the 70 Hz retrace |
| Zoom | about 20x10 tiles visible, camera follows the player; auto-tiled walls, sprites, spiked traps |
| Maps | five distinct 40x28 campaign maps live in `Simulador/tomb_of_mask.asm`, with compact chambers, tight corridors, and routes designed around wall-to-wall dashes; the extractor copies them into the DOS assembly and C reference data. Endless mode uses generated rooms with chambers, obstacles, and solver-checked placements |
| Endless pressure | a red lava trail climbs from below, advances every second, and ends the run when it reaches the player |
| Trap feedback | hitting an unshielded trap displays **STAGE FAILED** before restarting a normal stage or ending an endless run |
| Sound | PC speaker effects for coins, items, shields, levers, plates, doors, bombs, crates, traps, level clears and the win screen, plus a looping tomb theme on the AdLib/OPL2 channel, with separate SOUND FX and GAME MUSIC toggles in SETTINGS. Music and effects use different hardware, so they can play together without altering each other |
| Slow room change | the ICMC version redrew all 1200 cells 30-40 times and waited ~2.7M simulated instructions after each step. Here the change is one memory blit per frame (~0.3 s) and only changed frames are drawn. A patched `Simulador/tomb_of_mask.asm/.mif` (4 words changed) makes the old version ~4x faster too |

## Is it really assembly?
Yes: every file in `src/` is assembly (`.s` / `.inc`). The only generated parts are data tables
(sprites, font, palette, and map copies from `Simulador/tomb_of_mask.asm`) written as assembly data
by the small tools in `tools/`. The game itself contains no C.

GAME MUSIC uses the AdLib/OPL2 chip that DOSBox provides by default. SOUND FX stay on the PC
speaker, which is why both can be enabled at once. If a machine has no OPL2, the effects still
work and the music is silent.

## How it was tested
During development `make test` ran the assembly build against a C reference in a small 386 emulator
(`tests/x86run.c`) and compared frames and generated rooms. That reference implementation and test
harness are **not shipped** in this cleaned submission copy, so `make test` only works in the
development checkout.

**Smoke test:** the bundled `TOMB.COM` was launched in DOSBox 0.74-3 on Windows and reached the title
screen. The game logic was also exercised in a headless 386 emulator with randomized input.

## Layout
    dos/   TOMB.COM  RUN.BAT  tomb.conf  HOW_TO_RUN.md  Makefile  src/  tools/
    Simulador/   patched tomb_of_mask.asm / .mif for the original simulator (faster room change)

Edit the five grids in `dos/campaign_maps.txt`. Each map is 28 rows by 40 columns:
`#` is a wall, `.` is floor, `C` is a crate, and `S`/`E` mark the player start and exit.
Other tiles use their in-game symbols, such as `*` for a coin, `{` for a trap, `$` for a key,
`=` for a door, `&` for a bomb, and `|` for a breakable wall. Keep every row exactly 40 characters
wide. The compact simulator strings and DOS map data are generated from these grids.

After editing a grid, regenerate the map data from `dos/`:

    python3 tools/extract_levels_asm.py campaign_maps.txt ../Simulador/tomb_of_mask.asm src/levels.inc reference_c/src/levels.h

This also refreshes the compact map strings and start/exit metadata in `Simulador/tomb_of_mask.asm`.
`make levels` selects `py -3` on Windows and `python3` on Linux/WSL automatically.
