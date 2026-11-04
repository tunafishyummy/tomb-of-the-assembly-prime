# ============================================================================
#  TOMB OF THE ASSEMBLY - DOS edition
#  Pure x86 assembly (16-bit real mode, 80386+ instructions), builds a .COM file.
#  Assemble: as --32 -march=i386 -I src src/tomb.s -o build/tomb.o
#            ld -m elf_i386 -T src/tomb.ld -o TOMB.COM build/tomb.o
# ============================================================================
    .intel_syntax noprefix
    .code16
    .include "defs.inc"
    .include "assets.inc"
    .include "levels.inc"

    .section .text.start, "ax", @progbits
    .globl _start
_start:
    cld
    mov di, offset __bss_start       # DOS does not clear memory: zero our variables
    mov cx, offset __bss_end
    sub cx, di
    xor al, al
    rep stosb
    call init_trail_tile
    call init_pellet_tile
    call game_main
    mov ax, 0x4C00                   # back to DOS
    int 0x21

    .text
    .include "plat.inc"
    .include "gfx.inc"
    .include "engine.inc"
    .include "gen.inc"
    .include "ui.inc"

# Build the tiny yellow collectible pellet used along the corridors.
init_pellet_tile:
    mov di, offset pellet_tile
    mov cx, 256
    mov al, P_BLACK
    rep stosb
    mov byte ptr [pellet_tile+7*16+7], P_YELLOW
    mov byte ptr [pellet_tile+7*16+8], P_YELLOW
    mov byte ptr [pellet_tile+8*16+7], P_YELLOW
    mov byte ptr [pellet_tile+8*16+8], P_YELLOW
    ret

# Build a tiny yellow square on black for the persistent dash trail.
init_trail_tile:
    mov di, offset trail_tile
    mov cx, 256
    mov al, P_BLACK
    rep stosb
    mov bx, 6
.Litt_y:
    cmp bx, 10
    jge .Litt_done
    mov si, bx
    shl si, 4
    add si, 6
    mov cx, 4
.Litt_x:
    mov byte ptr [trail_tile+si], P_YELLOW
    inc si
    loop .Litt_x
    inc bx
    jmp .Litt_y
.Litt_done:
    ret

# ----------------------------------------------------------------------------
#  Text
# ----------------------------------------------------------------------------
msg_nomem:       .ascii "NOT ENOUGH MEMORY\r\n$"
str_empty:       .asciz ""
str_dash:        .asciz "-"
str_cursor:      .asciz ">"
str_room:        .asciz "ROOM "
str_lvl:         .asciz "LVL "
str_hint:        .asciz "WASD/ARROWS DASH  B BOMB  R RESTART  H HELP  M MENU"
str_clear:       .asciz "LEVEL COMPLETE"
str_next_option: .asciz "> CONTINUE TO NEXT LEVEL"
str_select_option:.asciz "  RETURN TO STAGE SELECTION"
str_gameover:    .asciz "GAME OVER"
str_stage_failed:.asciz "STAGE FAILED"
str_lava:        .asciz "LAVA RISING! KEEP CLIMBING"
str_win:         .asciz "YOU WIN!"
str_shield_bonus:.asciz "SHIELD ALREADY ACTIVE +5 COINS"
str_trap:        .asciz "TRAP HIT"
str_failed_retry:.asciz "> RETRY STAGE"
str_failed_select:.asciz "  STAGE SELECTION"
str_failed_hint:.asciz "UP/DOWN SELECT   ENTER CONFIRM"
str_nobombs:     .asciz "NO BOMBS LEFT"
str_nothing:     .asciz "NOTHING TO BREAK"
str_title1:      .asciz "TOMB OF"
str_title2:      .asciz "THE MASK"
str_credit:      .asciz "DOS EDITION - GROUP 1 FINAL PROJECT"
str_menu_hint:   .asciz "ARROWS/WASD MOVE   ENTER SELECT   ESC QUIT"
str_item0:       .asciz "STAGE SELECT"
str_item1:       .asciz "ENDLESS MODE"
str_item2:       .asciz "SETTINGS"
str_settings:    .asciz "SETTINGS"
str_stage_title: .asciz "SELECT STAGE"
str_stage_hint:  .asciz "ARROWS MOVE   ENTER PLAY   ESC BACK"
str_stage_label: .asciz "FIVE MAPS"
str_set0s:       .asciz "> DASH SPEED"
str_set0:        .asciz "  DASH SPEED"
str_set1s:       .asciz "> SOUND FX"
str_set1:        .asciz "  SOUND FX"
str_set2s:       .asciz "> GAME MUSIC"
str_set2:        .asciz "  GAME MUSIC"
str_set3s:       .asciz "> BACK"
str_set3:        .asciz "  BACK"
str_spd0:        .asciz "SLOW  "
str_spd1:        .asciz "NORMAL"
str_spd2:        .asciz "FAST  "
str_reserved:    .asciz "RESERVED"
str_sound_note:  .asciz "SOUND IS RESERVED FOR A FUTURE UPDATE"
str_set_hint:    .asciz "UP/DOWN MOVE  LEFT/RIGHT CHANGE  ENTER BACK"
str_legend:      .asciz "ICON LEGEND"
str_anykey:      .asciz "ANY KEY TO RETURN"
str_rooms:       .asciz "ROOMS CLEARED:"
str_coins:       .asciz "COINS:"
str_enter:       .asciz "PRESS ENTER"
str_h0:          .asciz "PLAYER"
str_h1:          .asciz "COIN"
str_h2:          .asciz "TRAP - DEADLY"
str_h3:          .asciz "WALL"
str_h4:          .asciz "CRATE - PUSH IT"
str_h5:          .asciz "KEY"
str_h6:          .asciz "DOOR - NEEDS KEY"
str_h7:          .asciz "PLATE - OPENS GATES"
str_h8:          .asciz "GATE"
str_h9:          .asciz "LEVER - TOGGLES WALLS"
str_h10:         .asciz "BOMB (B)"
str_h11:         .asciz "SHIELD"
str_h12:         .asciz "WEAK WALL - BOMB IT"
str_h13:         .asciz "LEVER WALL"

menu_items:      .word offset str_item0, offset str_item1, offset str_item2
set_items:       .word offset str_set0s, offset str_set0, offset str_set1s, offset str_set1
                 .word offset str_set2s, offset str_set2, offset str_set3s, offset str_set3
speed_names:     .word offset str_spd0, offset str_spd1, offset str_spd2
speed_px:        .word 6, 10, 16                 # pixels per frame: slow, normal, fast
help_rows:       .byte 122, 0
                 .word offset str_h0
                 .byte T_COIN, 0
                 .word offset str_h1
                 .byte T_TRAP, 0
                 .word offset str_h2
                 .byte T_WALL, 0
                 .word offset str_h3
                 .byte T_CRATE, 0
                 .word offset str_h4
                 .byte T_KEY, 0
                 .word offset str_h5
                 .byte T_DOOR, 0
                 .word offset str_h6
                 .byte T_PLATE, 0
                 .word offset str_h7
                 .byte T_GATE, 0
                 .word offset str_h8
                 .byte T_LEVER, 0
                 .word offset str_h9
                 .byte T_BOMB, 0
                 .word offset str_h10
                 .byte T_SHIELD, 0
                 .word offset str_h11
                 .byte T_BREAK, 0
                 .word offset str_h12
                 .byte T_TOGGLE, 0
                 .word offset str_h13

# ----------------------------------------------------------------------------
#  Variables (zeroed at start-up)
# ----------------------------------------------------------------------------
    .bss
backseg:    .space 2
seed:       .space 4
frame:      .space 2
gfx_dirty:  .space 2
clip_x1:    .space 2
clip_y1:    .space 2
r_col:      .space 2
r_w:        .space 2
r_x:        .space 2
r_y:        .space 2
r_rows:     .space 2
b_x:        .space 2
b_y:        .space 2
b_key:      .space 2
b_src:      .space 2
b_x0:       .space 2
b_xe:       .space 2
b_row:      .space 2
gl_x:       .space 2
gl_y:       .space 2
gl_col:     .space 2
gl_sc:      .space 2
gl_ptr:     .space 2
gl_r:       .space 2
gl_bits:    .space 2
gl_c:       .space 2
gl_e:       .space 2
gl_k:       .space 2
tx_x:       .space 2
tx_y:       .space 2
tx_col:     .space 2
tx_sc:      .space 2
tx_ptr:     .space 2
num_buf:    .space 8
om_x:       .space 2
om_y:       .space 2
bt_mask:    .space 2
bt_h:       .space 2
tf_x:       .space 2
tf_y:       .space 2
gr_room:    .space 2
gr_ox:      .space 2
gr_oy:      .space 2
gr_exit:    .space 2
gr_tx0:     .space 2
gr_ty0:     .space 2
gr_tx1:     .space 2
gr_ty1:     .space 2
gr_tx:      .space 2
gr_ty:      .space 2
gr_sx:      .space 2
gr_sy:      .space 2
trap_tile:  .space 256
trail_tile: .space 256
pellet_tile: .space 256
hud_label:  .space 16
g_px:       .space 2
g_py:       .space 2
g_trail_x:  .space 2
g_trail_y:  .space 2
g_dx:       .space 2
g_dy:       .space 2
g_prog:     .space 2
g_to_exit:  .space 2
g_coins:    .space 2
g_coin_rec: .space 2
g_keys:     .space 2
g_bombs:    .space 2
g_shield:   .space 2
g_mode:     .space 2
g_level:    .space 2
g_depth:    .space 2
g_msg:      .space 2
g_msg_t:    .space 2
g_lava_y:   .space 2
g_lava_clock: .space 2
g_fail_select: .space 2
lava_draw_y: .space 2
lava_band_y: .space 2
failed_sel: .space 2
ev_acc:     .space 2
bs_nx:      .space 2
bs_ny:      .space 2
gt_px:      .space 2
rl_dst:     .space 2
rl_idx:     .space 2
cam_x:      .space 2
cam_y:      .space 2
cam_tx:     .space 2
cam_ty:     .space 2
speed_idx:  .space 2
last_valid: .space 2
dw_px:      .space 2
dw_py:      .space 2
trail_draw_x: .space 2
trail_draw_y: .space 2
trail_draw_w: .space 2
trail_draw_h: .space 2
dw_a:       .space 2
dw_b:       .space 2
dw_hud:     .space 4
lcx:        .space 2
lcy:        .space 2
lpx:        .space 2
lpy:        .space 2
la:         .space 2
lb:         .space 2
lhud:       .space 4
lmsg:       .space 2
tb_y:       .space 2
m_sel:      .space 2
m_redraw:   .space 2
m_i:        .space 2
m_y:        .space 2
m_key:      .space 2
stage_sel:  .space 2
stage_col:  .space 2
stage_row:  .space 2
s_note:     .space 2
hs_x:       .space 2
hs_y:       .space 2
e_rooms:    .space 2
e_coins:    .space 2
pf_n:       .space 2
st_dx:      .space 2
st_dy:      .space 2
st_total:   .space 2
st_nsx:     .space 2
st_nsy:     .space 2
st_ncx:     .space 2
st_ncy:     .space 2
st_p:       .space 2
st_q:       .space 2
st_oox:     .space 2
st_ooy:     .space 2
st_nox:     .space 2
st_noy:     .space 2
p_ev:       .space 2
p_k:        .space 2
p_dirx:     .space 2
p_diry:     .space 2
    .include "gen_vars.inc"
room:       .space ROOMSZ
orig:       .space ROOMSZ
next_room:  .space ROOMSZ
