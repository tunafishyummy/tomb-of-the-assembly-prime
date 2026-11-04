# How to run Tomb of the Assembly (DOS edition) - step by step

You do **not** need to build anything. `TOMB.COM` is already assembled from the `.s` source files.
You only need DOSBox, a free program that pretends to be an old DOS computer.

---------------------------------------------------------------------------

## Part 1 - Windows (easiest way)

### Step 1: Install DOSBox
1. Open your browser and go to **https://www.dosbox.com** and click **Download**.
2. Download the **Windows installer** (version 0.74-3 is fine; DOSBox Staging and DOSBox-X also work).
3. Run the installer, click **Next** through the screens, and finish. Keep the default install folder.

### Step 2: Unzip the game
1. Right-click `tomb-dos-edition.zip` and choose **Extract All...**
2. Pick a simple folder without strange characters, for example `C:\Tomb`.
3. Open the extracted folder, then open the folder named **`dos`**. You should see these files:
   `TOMB.COM`, `RUN.BAT`, `tomb.conf`, `HOW_TO_RUN.md`, and a `src` folder.

### Step 3: Start the game
1. Double-click **`RUN.BAT`**.
2. A DOSBox window opens, starts `TOMB.COM` automatically and you see the title screen
   (yellow "TOMB OF THE MASK", with NORMAL MODE / ENDLESS MODE / SETTINGS).
3. If Windows shows a blue "Windows protected your PC" box, click **More info** then **Run anyway**
   (it is only a small batch file that starts DOSBox).

If `RUN.BAT` says **"DOSBox not found"**, use Part 2 below instead.

### Step 4: Play
| Key | What it does |
|---|---|
| **W A S D** or **arrow keys** | Dash in that direction. You slide until you hit something. |
| **B** | Use a bomb on the weak walls next to you |
| **R** (or the ` key) | Restart the current room |
| **H** or **Tab** | Show the icon legend |
| **M** or **Esc** | Back to the menu |
| **Enter** | Select in menus |

Menu: use **W/S** or **Up/Down** to choose, **Enter** to start.
* **NORMAL MODE** - the 5 original levels.
* **ENDLESS MODE** - generated tombs that get bigger. Touching a trap ends the run (a shield saves you once).
* **SETTINGS** - dash speed (slow / normal / fast).

To quit: go to the title menu and press **Esc**. The DOSBox window then closes.

---------------------------------------------------------------------------

## Part 2 - Start it by hand inside DOSBox (if RUN.BAT does not work)

1. Start **DOSBox** from the Start menu. A black window with `Z:\>` opens.
2. Type this and press Enter (change the path to where your `dos` folder is):

       mount c C:\Tomb\dos

   You should see "Drive C is mounted as local directory ...".
   If the path has spaces, put it in quotes: `mount c "C:\My Games\Tomb\dos"`.
3. Type `c:` and press Enter. The prompt changes to `C:\>`.
4. Type `TOMB.COM` and press Enter. The game starts.
5. **If the game feels slow or choppy**, quit it, then at the `C:\>` prompt type `cycles=max`
   and press Enter, then start `TOMB.COM` again. (You can also press **Ctrl+F12** a few times
   while the game runs to raise the speed.)

---------------------------------------------------------------------------

## Part 3 - Mac / Linux

1. Install DOSBox (Mac: download from dosbox.com or `brew install dosbox-x`; Linux: `sudo apt install dosbox`).
2. Open a terminal in the `dos` folder and run:

       dosbox -conf tomb.conf

   or do the manual steps of Part 2 with `mount c /path/to/dos`.

---------------------------------------------------------------------------

## Handy DOSBox tips
* **Alt+Enter** toggles full screen. **Ctrl+F10** frees the mouse. **Ctrl+F9** closes DOSBox.
* The window size is set in `tomb.conf` (`windowresolution=1280x800`); change it if it is too big or small.
* `tomb.conf` also sets `cycles=max`. That is important: DOSBox's default CPU speed is far too slow for the game.

## Troubleshooting
| Problem | Fix |
|---|---|
| Game is slow / choppy | Make sure you start it with `RUN.BAT` or `tomb.conf` (`cycles=max`). Close other heavy programs. |
| Black window, nothing happens | Wait 2 seconds, then press Enter. Make sure you typed `TOMB.COM` from the right folder (`dir` should list it). |
| "Illegal command: TOMB.COM" | You are not in the folder that holds it. Redo `mount c ...` with the path of the `dos` folder, then `c:`. |
| "NOT ENOUGH MEMORY" | In `tomb.conf` the `[dosbox]` default memory is fine; do not lower `memsize` below 4. |
| Keys do nothing | Click on the DOSBox window first so it has the keyboard focus. |
| Window is tiny | Edit `windowresolution=` in `tomb.conf`, or press Alt+Enter for full screen. |

---------------------------------------------------------------------------

## Part 4 - Optional: build `TOMB.COM` yourself from the assembly source

The program is written in **x86 assembly (16-bit real mode, 80386 instructions)** using GNU `as`
in Intel syntax. The output is a flat DOS `.COM` file.

### On Windows (using WSL, Windows Subsystem for Linux)
1. Open **PowerShell as Administrator** and run `wsl --install`, then restart the PC and finish the
   Ubuntu setup (pick a user name and password).
2. Open the Ubuntu terminal and run: `sudo apt update && sudo apt install -y binutils make`
3. In Windows Explorer open the `dos` folder, click the address bar, type `cmd`, press Enter, then run
   `BUILD_WITH_WSL.bat`  (or inside Ubuntu: `cd /mnt/c/Tomb/dos` and `make`).
4. A fresh `TOMB.COM` appears. Run it as in Part 1.

### On Linux / macOS (with an x86 binutils)
    cd dos
    make            # or by hand:
    as --32 -march=i386 -I src src/tomb.s -o build/tomb.o
    ld -m elf_i386 -T src/tomb.ld -o TOMB.COM build/tomb.o

### Where things are in the source
| File | Content |
|---|---|
| `src/tomb.s` | entry point (`_start`), text strings, variables; includes the rest |
| `src/defs.inc` | constants, room record layout |
| `src/plat.inc` | VGA mode 13h, double buffer, vertical-retrace sync, BIOS keyboard/timer, random numbers |
| `src/gfx.inc` | tile blitting with clipping, text, auto-tiled walls, spiked traps, HUD |
| `src/engine.inc` | game rules: dash, coins, keys, doors, crates, plates, levers, bombs, shield, traps |
| `src/gen.inc` | endless room generator + solver |
| `src/ui.inc` | menus, camera, room-change scroll, main loop |
| `src/assets.inc`, `src/levels.inc` | generated data tables (sprites, font, palette, the 5 levels) |
