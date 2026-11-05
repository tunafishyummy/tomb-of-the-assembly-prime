# ============================================================================
#  Constants and room layout
# ============================================================================
    .equ MAXW, 40
    .equ MAXH, 52
    .equ MAXCELLS, 2080              # MAXW*MAXH
    .equ NUM_LEVELS_UNUSED, 5

# --- Room record
    .equ R_W, 0                      # width  (tiles)
    .equ R_H, 2                      # height (tiles)
    .equ R_SX, 4                     # player start
    .equ R_SY, 6
    .equ R_EX, 8                     # exit cell (opening in the outer wall)
    .equ R_EY, 10
    .equ R_NFLIP, 12                 # number of lever targets
    .equ R_FLIP, 14                  # 4 words: cell indices toggled by the lever
    .equ R_C, 22                     # MAXCELLS bytes of tiles
    .equ ROOMSZ, 2102                # 22 + MAXCELLS

# --- Tiles
    .equ T_FLOOR, 32                 # ' '
    .equ T_WALL, 125                 # '}'
    .equ T_TRAP, 123                 # '{'
    .equ T_COIN, 42                  # '*'
    .equ T_CRATE, 35                 # '#'
    .equ T_KEY, 36                   # '$'
    .equ T_DOOR, 61                  # '='
    .equ T_PLATE, 64                 # '@'
    .equ T_GATE, 37                  # '%'
    .equ T_LEVER, 60                 # '<'
    .equ T_BOMB, 38                  # '&'
    .equ T_SHIELD, 94                # '^'
    .equ T_BREAK, 124                # '|'
    .equ T_TOGGLE, 33                # '!'
    .equ T_TRAIL, 96                 # yellow dash trail left behind the player

# --- Screen / camera
    .equ TILE, 16
    .equ SCR_W, 320
    .equ SCR_H, 200
    .equ VIEW_W, 320
    .equ VIEW_H, 168                 # game view, the HUD uses the 32 lines below

# --- Keys
    .equ K_NONE, 0
    .equ K_UP, 1001
    .equ K_DOWN, 1002
    .equ K_LEFT, 1003
    .equ K_RIGHT, 1004
    .equ K_ENTER, 13
    .equ K_ESC, 27
    .equ K_TAB, 9

# --- Modes, events
    .equ MODE_NORMAL, 1
    .equ MODE_ENDLESS, 2
    .equ EV_CLEAR, 1
    .equ EV_DEAD, 2
    .equ EV_COIN, 4
    .equ EV_ITEM, 8
    .equ NUM_LEVELS, 5