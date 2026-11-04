# ============================================================================
#  Game flow: frame timing, camera, menus, room transitions, main loop.
# ============================================================================
    .text
# key classifiers: AX = key -> DL = 1 / 0 (AX preserved)
is_up:
    mov dl, 1
    cmp ax, K_UP
    je .Liu
    cmp ax, 119                      # 'w'
    je .Liu
    cmp ax, 87                       # 'W'
    je .Liu
    xor dl, dl
.Liu:
    ret
is_down:
    mov dl, 1
    cmp ax, K_DOWN
    je .Lid
    cmp ax, 115
    je .Lid
    cmp ax, 83
    je .Lid
    xor dl, dl
.Lid:
    ret
is_left:
    mov dl, 1
    cmp ax, K_LEFT
    je .Lil
    cmp ax, 97
    je .Lil
    cmp ax, 65
    je .Lil
    xor dl, dl
.Lil:
    ret
is_right:
    mov dl, 1
    cmp ax, K_RIGHT
    je .Lir
    cmp ax, 100
    je .Lir
    cmp ax, 68
    je .Lir
    xor dl, dl
.Lir:
    ret

# tick: wait for the retrace, then show whatever was drawn since the last frame
tick:
    call plat_wait_frame
    cmp byte ptr [gfx_dirty], 0
    je .Lt1
    call plat_present
    mov byte ptr [gfx_dirty], 0
.Lt1:
    inc word ptr [frame]
    ret

invalidate:
    mov word ptr [last_valid], 0
    ret

# wait_key: block until a key is pressed, AX = key
wait_key:
    call plat_flush_keys
.Lwk1:
    call tick
    call plat_key
    test ax, ax
    jz .Lwk1
    call seed_mix
    ret

# clamp_cam: AX=wanted scroll, BX=map size px, CX=view size px -> AX (small maps are centred)
clamp_cam:
    mov dx, bx
    sub dx, cx                       # max scroll
    jg .Lcc1
    mov ax, dx
    cwd
    mov cx, 2
    idiv cx
    ret
.Lcc1:
    test ax, ax
    jns .Lcc2
    xor ax, ax
    ret
.Lcc2:
    cmp ax, dx
    jle .Lcc3
    mov ax, dx
.Lcc3:
    ret

# cam_target: -> [cam_tx],[cam_ty] centred on the (animated) player position
cam_target:
    mov ax, word ptr [g_px]
    shl ax, 4
    mov bx, word ptr [g_dx]
    imul bx, word ptr [g_prog]
    add ax, bx
    add ax, 8
    sub ax, VIEW_W/2
    mov bx, word ptr [room+R_W]
    shl bx, 4
    mov cx, VIEW_W
    call clamp_cam
    mov word ptr [cam_tx], ax
    mov ax, word ptr [g_py]
    shl ax, 4
    mov bx, word ptr [g_dy]
    imul bx, word ptr [g_prog]
    add ax, bx
    add ax, 8
    sub ax, VIEW_H/2
    mov bx, word ptr [room+R_H]
    shl bx, 4
    mov cx, VIEW_H
    call clamp_cam
    mov word ptr [cam_ty], ax
    ret

# ease: AX=current, BX=target -> AX moves a third of the way (snaps when close)
ease:
    mov cx, bx
    sub cx, ax                       # d
    jz .Le_ret
    cmp cx, -4
    jle .Le1
    cmp cx, 4
    jge .Le1
    mov ax, bx
    ret
.Le1:
    push ax
    mov ax, cx
    cwd
    mov cx, 3
    idiv cx
    mov cx, ax
    pop ax
    add ax, cx
.Le_ret:
    ret

setup_room_camera:
    call cam_target
    mov ax, word ptr [cam_tx]
    mov word ptr [cam_x], ax
    mov ax, word ptr [cam_ty]
    mov word ptr [cam_y], ax
    ret

# draw_world: repaint only what changed since the last call
draw_world:
    mov ax, word ptr [g_px]
    shl ax, 4
    mov bx, word ptr [g_dx]
    imul bx, word ptr [g_prog]
    add ax, bx
    mov word ptr [dw_px], ax         # player pixel position
    mov ax, word ptr [g_py]
    shl ax, 4
    mov bx, word ptr [g_dy]
    imul bx, word ptr [g_prog]
    add ax, bx
    mov word ptr [dw_py], ax
    mov ax, word ptr [frame]         # a = (frame/10)&1, b = (frame/14)&1
    xor dx, dx
    mov cx, 10
    div cx
    and ax, 1
    mov word ptr [dw_a], ax
    mov ax, word ptr [frame]
    xor dx, dx
    mov cx, 14
    div cx
    and ax, 1
    mov word ptr [dw_b], ax
    movzx eax, word ptr [g_coins]    # hud signature (same packing as the C version)
    movzx ebx, word ptr [g_keys]
    shl ebx, 10
    or eax, ebx
    movzx ebx, word ptr [g_bombs]
    shl ebx, 14
    or eax, ebx
    movzx ebx, word ptr [g_shield]
    shl ebx, 18
    or eax, ebx
    cmp word ptr [g_msg_t], 0
    jle .Ldw0
    or eax, 0x80000                  # (msg_t > 0) << 19
.Ldw0:
    mov dword ptr [dw_hud], eax
    cmp word ptr [last_valid], 0
    je .Ldw_redraw
    mov ax, word ptr [cam_x]
    cmp ax, word ptr [lcx]
    jne .Ldw_redraw
    mov ax, word ptr [cam_y]
    cmp ax, word ptr [lcy]
    jne .Ldw_redraw
    mov ax, word ptr [dw_px]
    cmp ax, word ptr [lpx]
    jne .Ldw_redraw
    mov ax, word ptr [dw_py]
    cmp ax, word ptr [lpy]
    jne .Ldw_redraw
    mov ax, word ptr [dw_a]
    cmp ax, word ptr [la]
    jne .Ldw_redraw
    mov ax, word ptr [dw_b]
    cmp ax, word ptr [lb]
    je .Ldw_hud
.Ldw_redraw:
    mov al, P_BLACK
    call gfx_clear_view
    mov si, offset room
    mov bx, word ptr [cam_x]
    neg bx
    mov cx, word ptr [cam_y]
    neg cx
    mov dl, 1
    call gfx_room
    cmp word ptr [g_mode], MODE_ENDLESS
    jne .Ldw_no_lava
    mov ax, word ptr [g_lava_y]
    cmp ax, 0
    jl .Ldw_no_lava
    cmp ax, word ptr [room+R_H]
    jge .Ldw_no_lava
    # Draw a layered lava front: a bright leading line plus several hot bands.
    shl ax, 4
    sub ax, word ptr [cam_y]
    mov word ptr [lava_draw_y], ax
    mov bx, 0
    mov cx, word ptr [lava_draw_y]
    mov si, SCR_W
    mov di, 5
    mov dl, P_TRAP
    call gfx_rect
    mov bx, 0
    mov cx, word ptr [lava_draw_y]
    add cx, 5
    mov si, SCR_W
    mov di, 5
    mov dl, P_RED_HI
    call gfx_rect
    mov bx, 0
    mov cx, word ptr [lava_draw_y]
    add cx, 10
    mov si, SCR_W
    mov di, 6
    mov dl, P_TRAP
    call gfx_rect
    mov bx, 0
    mov cx, word ptr [lava_draw_y]
    add cx, 16
    mov si, SCR_W
    mov di, 8
    mov dl, P_RED
    call gfx_rect
    # Keep layered lava bands filling the lower screen, not just one scanline.
    mov ax, word ptr [lava_draw_y]
    add ax, 24
    mov word ptr [lava_band_y], ax
.Ldw_lava_bands:
    mov ax, word ptr [lava_band_y]
    cmp ax, VIEW_H
    jge .Ldw_no_lava
    mov bx, 0
    mov cx, ax
    mov si, SCR_W
    mov di, 8
    mov dl, P_TRAP
    test ax, 8
    jz .Ldw_lava_color
    mov dl, P_RED
.Ldw_lava_color:
    call gfx_rect
    add word ptr [lava_band_y], 8
    jmp .Ldw_lava_bands
.Ldw_no_lava:
    # Draw a thin yellow line from the dash origin to the moving player.
    # It is an overlay only; the room tiles are never modified, so it vanishes on stop.
    mov ax, word ptr [g_dx]
    or ax, word ptr [g_dy]
    jz .Ldw_trail_done
    mov ax, word ptr [g_trail_x]
    sub ax, word ptr [cam_x]
    add ax, 8
    mov word ptr [trail_draw_x], ax
    mov ax, word ptr [g_trail_y]
    sub ax, word ptr [cam_y]
    add ax, 8
    mov word ptr [trail_draw_y], ax
    mov ax, word ptr [dw_px]
    sub ax, word ptr [cam_x]
    add ax, 8
    mov word ptr [trail_draw_w], ax
    mov ax, word ptr [dw_py]
    sub ax, word ptr [cam_y]
    add ax, 8
    mov word ptr [trail_draw_h], ax
    cmp word ptr [g_dx], 0
    je .Ldw_vertical_trail
    # Horizontal line: y is the current dash row, x spans origin to player.
    mov ax, word ptr [trail_draw_x]
    mov bx, word ptr [trail_draw_w]
    cmp ax, bx
    jle .Ldw_htrail_ordered
    xchg ax, bx
.Ldw_htrail_ordered:
    mov word ptr [trail_draw_x], ax
    sub bx, ax
    inc bx
    mov word ptr [trail_draw_w], bx
    mov bx, ax
    mov cx, word ptr [trail_draw_y]
    dec cx
    mov si, word ptr [trail_draw_w]
    mov di, 2
    mov dl, P_YELLOW
    call gfx_rect
    jmp .Ldw_trail_done
.Ldw_vertical_trail:
    # Vertical line: x is the current dash column, y spans origin to player.
    mov ax, word ptr [trail_draw_y]
    mov bx, word ptr [trail_draw_h]
    cmp ax, bx
    jle .Ldw_vtrail_ordered
    xchg ax, bx
.Ldw_vtrail_ordered:
    mov word ptr [trail_draw_y], ax
    sub bx, ax
    inc bx
    mov word ptr [trail_draw_h], bx
    mov bx, word ptr [trail_draw_x]
    dec bx
    mov cx, ax
    mov si, 2
    mov di, word ptr [trail_draw_h]
    mov dl, P_YELLOW
    call gfx_rect
.Ldw_trail_done:
    mov bx, word ptr [dw_px]
    sub bx, word ptr [cam_x]
    mov cx, word ptr [dw_py]
    sub cx, word ptr [cam_y]
    call gfx_player
    mov ax, word ptr [cam_x]
    mov word ptr [lcx], ax
    mov ax, word ptr [cam_y]
    mov word ptr [lcy], ax
    mov ax, word ptr [dw_px]
    mov word ptr [lpx], ax
    mov ax, word ptr [dw_py]
    mov word ptr [lpy], ax
    mov ax, word ptr [dw_a]
    mov word ptr [la], ax
    mov ax, word ptr [dw_b]
    mov word ptr [lb], ax
.Ldw_hud:
    cmp word ptr [last_valid], 0
    je .Ldw_hud_do
    mov eax, dword ptr [dw_hud]
    cmp eax, dword ptr [lhud]
    jne .Ldw_hud_do
    mov ax, word ptr [g_msg]
    cmp ax, word ptr [lmsg]
    je .Ldw_done
.Ldw_hud_do:
    call gfx_hud
    mov eax, dword ptr [dw_hud]
    mov dword ptr [lhud], eax
    mov ax, word ptr [g_msg]
    mov word ptr [lmsg], ax
.Ldw_done:
    mov word ptr [last_valid], 1
    ret

# title_bg: black screen with a wall band at the top and bottom
title_bg:
    mov al, P_BLACK
    call gfx_clear_all
    mov word ptr [tb_y], -8
    call .Ltb_row
    mov word ptr [tb_y], 8
    call .Ltb_row
    mov word ptr [tb_y], SCR_H-24
    call .Ltb_row
    mov word ptr [tb_y], SCR_H-8
.Ltb_row:
    xor si, si
.Ltb1:
    mov al, T_WALL
    mov bx, si
    shl bx, 4
    mov cx, word ptr [tb_y]
    push si
    call gfx_sprite_tile
    pop si
    inc si
    cmp si, 20
    jl .Ltb1
    ret

# menu_screen: AX = 0 normal, 1 endless, 2 settings, -1 quit
menu_screen:
    mov word ptr [m_sel], 0
    mov word ptr [m_redraw], 1
    call plat_flush_keys
.Lm_loop:
    cmp word ptr [m_redraw], 0
    je .Lm_tick
    mov word ptr [m_redraw], 0
    call title_bg
    mov si, offset str_title1
    mov cx, 30
    mov dl, P_YELLOW
    mov dh, 3
    call gfx_text_c
    mov si, offset str_title2
    mov cx, 56
    mov dl, P_YELLOW
    mov dh, 3
    call gfx_text_c
    mov si, offset str_credit
    mov cx, 86
    mov dl, P_WALL_HI
    mov dh, 1
    call gfx_text_c
    mov word ptr [m_i], 0
.Lm_items:
    mov bx, word ptr [m_i]
    shl bx, 1
    mov si, word ptr [menu_items+bx]
    mov ax, word ptr [m_i]
    imul ax, ax, 18
    add ax, 102
    mov word ptr [m_y], ax
    mov dl, P_GREY
    mov ax, word ptr [m_i]
    cmp ax, word ptr [m_sel]
    jne .Lm_c
    mov dl, P_WHITE
.Lm_c:
    mov cx, word ptr [m_y]
    mov dh, 2
    push si
    call gfx_text_c
    pop si
    mov ax, word ptr [m_i]
    cmp ax, word ptr [m_sel]
    jne .Lm_next
    mov dh, 2                        # cursor
    call gfx_text_w
    mov bx, SCR_W
    sub bx, ax
    sar bx, 1
    sub bx, 16
    mov si, offset str_cursor
    mov cx, word ptr [m_y]
    mov dl, P_YELLOW
    mov dh, 2
    call gfx_text
.Lm_next:
    inc word ptr [m_i]
    cmp word ptr [m_i], 3
    jl .Lm_items
    mov si, offset str_menu_hint
    mov cx, 162
    mov dl, P_GREY
    mov dh, 1
    call gfx_text_c
    mov al, 122                      # 'z'
    mov bx, 24
    mov cx, 96
    call gfx_sprite_tile
.Lm_tick:
    call tick
    call plat_key
    test ax, ax
    jz .Lm_loop
    mov word ptr [m_redraw], 1
    mov word ptr [m_key], ax
    call seed_mix
    mov ax, word ptr [m_key]
    call is_up
    test dl, dl
    jz .Lm_k1
    mov ax, word ptr [m_sel]
    add ax, 2
    jmp .Lm_mod3
.Lm_k1:
    mov ax, word ptr [m_key]
    call is_down
    test dl, dl
    jz .Lm_k2
    mov ax, word ptr [m_sel]
    add ax, 1
.Lm_mod3:
    xor dx, dx
    mov cx, 3
    div cx
    mov word ptr [m_sel], dx
    jmp .Lm_loop
.Lm_k2:
    cmp word ptr [m_key], K_ENTER
    jne .Lm_k3
    mov ax, word ptr [m_sel]
    ret
.Lm_k3:
    cmp word ptr [m_key], K_ESC
    jne .Lm_loop
    mov ax, -1
    ret

# stage_select: five-stage row; AX = selected stage index (0..4), -1 = back
stage_select:
    mov word ptr [stage_sel], 0
    mov word ptr [m_redraw], 1
    call plat_flush_keys
.Lss_loop:
    cmp word ptr [m_redraw], 0
    je .Lss_tick
    mov word ptr [m_redraw], 0
    call title_bg
    mov si, offset str_stage_title
    mov cx, 18
    mov dl, P_YELLOW
    mov dh, 3
    call gfx_text_c
    mov si, offset str_stage_label
    mov cx, 43
    mov dl, P_WALL_HI
    mov dh, 1
    call gfx_text_c
    mov word ptr [m_i], 0
.Lss_items:
    mov ax, word ptr [m_i]
    xor dx, dx
    mov bx, 5
    div bx
    mov word ptr [stage_row], ax
    mov word ptr [stage_col], dx
    # Stage tiles are a single row of the five authored maps.
    imul bx, dx, 52
    add bx, 32
    imul cx, ax, 18
    add cx, 61
    # Draw each stage as a small temple-stone tile; selected tile glows yellow.
    push bx
    push cx
    mov si, 38
    mov di, 14
    mov dl, P_WALL
    mov ax, word ptr [m_i]
    cmp ax, word ptr [stage_sel]
    jne .Lss_tile
    mov dl, P_YELLOW_LO
.Lss_tile:
    sub bx, 4
    sub cx, 1
    call gfx_rect
    pop cx
    pop bx
    mov ax, word ptr [m_i]
    inc ax
    mov di, 2
    mov dh, 2
    mov dl, P_GREY
    cmp word ptr [m_i], 9
    jg .Lss_num_color
    mov dl, P_WHITE
.Lss_num_color:
    cmp word ptr [m_i], 0
    # selected stage number is bright yellow
    mov ax, word ptr [m_i]
    cmp ax, word ptr [stage_sel]
    jne .Lss_num_plain
    mov dl, P_YELLOW
.Lss_num_plain:
    mov ax, word ptr [m_i]
    inc ax
    add bx, 9
    push bx
    push cx
    call gfx_num
    pop cx
    pop bx
    sub bx, 9
    # pixel-style brackets around the number, brighter when selected
    inc word ptr [m_i]
    cmp word ptr [m_i], NUM_LEVELS
    jl .Lss_items
    mov si, offset str_stage_hint
    mov cx, 171
    mov dl, P_GREY
    mov dh, 1
    call gfx_text_c
.Lss_tick:
    call tick
    call plat_key
    test ax, ax
    jz .Lss_loop
    mov word ptr [m_key], ax
    call seed_mix
    mov ax, word ptr [m_key]
    call is_up
    test dl, dl
    jz .Lss_down
    mov ax, word ptr [stage_sel]
    dec ax
    jns .Lss_set
    mov ax, NUM_LEVELS-1
    jmp .Lss_set
.Lss_down:
    mov ax, word ptr [m_key]
    call is_down
    test dl, dl
    jz .Lss_left
    mov ax, word ptr [stage_sel]
    inc ax
    cmp ax, NUM_LEVELS
    jl .Lss_set
    xor ax, ax
    jmp .Lss_set
.Lss_left:
    mov ax, word ptr [m_key]
    call is_left
    test dl, dl
    jz .Lss_right
    mov ax, word ptr [stage_sel]
    dec ax
    cmp ax, 0
    jge .Lss_set
    mov ax, NUM_LEVELS-1
    jmp .Lss_set
.Lss_right:
    mov ax, word ptr [m_key]
    call is_right
    test dl, dl
    jz .Lss_enter
    mov ax, word ptr [stage_sel]
    inc ax
    cmp ax, NUM_LEVELS
    jl .Lss_set
    xor ax, ax
.Lss_set:
    mov word ptr [stage_sel], ax
    mov word ptr [m_redraw], 1
    jmp .Lss_loop
.Lss_enter:
    cmp word ptr [m_key], K_ENTER
    jne .Lss_esc
    mov ax, word ptr [stage_sel]
    ret
.Lss_esc:
    cmp word ptr [m_key], K_ESC
    jne .Lss_loop
    mov ax, -1
    ret

# settings_screen
settings_screen:
    mov word ptr [m_sel], 0
    mov word ptr [m_redraw], 1
    mov word ptr [s_note], offset str_empty
    call plat_flush_keys
.Ls_loop:
    cmp word ptr [m_redraw], 0
    je .Ls_tick
    mov word ptr [m_redraw], 0
    call title_bg
    mov si, offset str_settings
    mov cx, 20
    mov dl, P_YELLOW
    mov dh, 3
    call gfx_text_c
    mov word ptr [m_i], 0
.Ls_items:                           # four option lines, y = 70 + 20*i
    mov bx, word ptr [m_i]
    shl bx, 2                        # two words per entry: selected text, plain text
    mov si, word ptr [set_items+bx+2]
    mov dl, P_GREY
    mov ax, word ptr [m_i]
    cmp ax, word ptr [m_sel]
    jne .Ls_c
    mov si, word ptr [set_items+bx]
    mov dl, P_WHITE
.Ls_c:
    mov cx, word ptr [m_i]
    imul cx, cx, 20
    add cx, 70
    mov bx, 40
    mov dh, 2
    call gfx_text
    inc word ptr [m_i]
    cmp word ptr [m_i], 4
    jl .Ls_items
    mov bx, word ptr [speed_idx]
    shl bx, 1
    mov si, word ptr [speed_names+bx]
    mov bx, 230
    mov cx, 70
    mov dl, P_YELLOW
    mov dh, 1
    call gfx_text
    mov si, offset str_reserved
    mov bx, 230
    mov cx, 94
    mov dl, P_GREY_LO
    mov dh, 1
    call gfx_text
    mov si, offset str_reserved
    mov bx, 230
    mov cx, 114
    mov dl, P_GREY_LO
    mov dh, 1
    call gfx_text
    mov si, word ptr [s_note]
    mov cx, 160
    mov dl, P_AQUA
    mov dh, 1
    call gfx_text_c
    mov si, offset str_set_hint
    mov cx, 162
    mov dl, P_GREY
    mov dh, 1
    call gfx_text_c
.Ls_tick:
    call tick
    call plat_key
    test ax, ax
    jz .Ls_loop
    mov word ptr [m_redraw], 1
    mov word ptr [m_key], ax
    mov word ptr [s_note], offset str_empty
    call is_up
    test dl, dl
    jz .Ls_k1
    mov ax, word ptr [m_sel]
    add ax, 3
    jmp .Ls_mod4
.Ls_k1:
    mov ax, word ptr [m_key]
    call is_down
    test dl, dl
    jz .Ls_k2
    mov ax, word ptr [m_sel]
    add ax, 1
.Ls_mod4:
    xor dx, dx
    mov cx, 4
    div cx
    mov word ptr [m_sel], dx
    jmp .Ls_loop
.Ls_k2:
    cmp word ptr [m_key], K_ESC
    jne .Ls_k3
    ret
.Ls_k3:
    cmp word ptr [m_sel], 0
    jne .Ls_k5
    mov ax, word ptr [m_key]
    call is_left
    test dl, dl
    jz .Ls_k4
    mov ax, word ptr [speed_idx]
    add ax, 2
    jmp .Ls_spd
.Ls_k4:
    mov ax, word ptr [m_key]
    call is_right
    test dl, dl
    jz .Ls_k5
    mov ax, word ptr [speed_idx]
    add ax, 1
.Ls_spd:
    xor dx, dx
    mov cx, 3
    div cx
    mov word ptr [speed_idx], dx
    jmp .Ls_loop
.Ls_k5:
    cmp word ptr [m_key], K_ENTER
    jne .Ls_loop
    cmp word ptr [m_sel], 0
    jne .Ls_e1
    mov ax, word ptr [speed_idx]
    inc ax
    xor dx, dx
    mov cx, 3
    div cx
    mov word ptr [speed_idx], dx
    jmp .Ls_loop
.Ls_e1:
    cmp word ptr [m_sel], 3
    jne .Ls_e2
    ret
.Ls_e2:
    mov word ptr [s_note], offset str_sound_note
    jmp .Ls_loop

# help_screen: icon legend
help_screen:
    mov al, P_BLACK
    call gfx_clear_all
    mov si, offset str_legend
    mov cx, 4
    mov dl, P_YELLOW
    mov dh, 2
    call gfx_text_c
    mov word ptr [m_i], 0
.Lhs_loop:
    mov ax, word ptr [m_i]           # col = i/7, row = i%7
    xor dx, dx
    mov cx, 7
    div cx
    imul ax, ax, 160
    add ax, 6
    mov word ptr [hs_x], ax
    imul dx, dx, 22
    add dx, 24
    mov word ptr [hs_y], dx
    mov bx, word ptr [m_i]
    shl bx, 2
    mov al, byte ptr [help_rows+bx]
    mov bx, word ptr [hs_x]
    mov cx, word ptr [hs_y]
    call gfx_sprite_tile
    mov bx, word ptr [m_i]
    shl bx, 2
    mov si, word ptr [help_rows+bx+2]
    mov bx, word ptr [hs_x]
    add bx, 22
    mov cx, word ptr [hs_y]
    add cx, 4
    mov dl, P_WHITE
    mov dh, 1
    call gfx_text
    inc word ptr [m_i]
    cmp word ptr [m_i], 14
    jl .Lhs_loop
    mov si, offset str_anykey
    mov cx, 186
    mov dl, P_GREY
    mov dh, 1
    call gfx_text_c
    call plat_present
    call wait_key
    ret

# end_screen: SI=title, [e_rooms] (-1 = hide), [e_coins]
end_screen:
    push si
    mov al, P_BLACK
    call gfx_clear_all
    pop si
    mov cx, 40
    mov dl, P_YELLOW
    mov dh, 3
    call gfx_text_c
    cmp word ptr [e_rooms], 0
    jl .Les1
    mov si, offset str_rooms
    mov bx, 80
    mov cx, 90
    mov dl, P_WHITE
    mov dh, 1
    call gfx_text
    mov ax, word ptr [e_rooms]
    mov di, 3
    mov bx, 190
    mov cx, 90
    mov dl, P_YELLOW
    mov dh, 1
    call gfx_num
.Les1:
    mov si, offset str_coins
    mov bx, 80
    mov cx, 104
    mov dl, P_WHITE
    mov dh, 1
    call gfx_text
    mov ax, word ptr [e_coins]
    mov di, 3
    mov bx, 190
    mov cx, 104
    mov dl, P_YELLOW
    mov dh, 1
    call gfx_num
    mov si, offset str_enter
    mov cx, 150
    mov dl, P_GREY
    mov dh, 1
    call gfx_text_c
.Les2:
    call wait_key
    cmp ax, K_ENTER
    jne .Les2
    ret

# stage_failed_screen: AX=0 retry, AX=1 return to stage selection.
stage_failed_screen:
    mov word ptr [failed_sel], 0
.Lsfs_draw:
    mov al, P_BLACK
    call gfx_clear_all
    mov si, offset str_stage_failed
    mov cx, 42
    mov dl, P_RED_HI
    mov dh, 3
    call gfx_text_c
    mov si, offset str_failed_retry
    mov cx, 86
    mov dl, P_YELLOW
    mov dh, 2
    cmp word ptr [failed_sel], 0
    je .Lsfs_retry
    mov dl, P_GREY
.Lsfs_retry:
    call gfx_text_c
    mov si, offset str_failed_select
    mov cx, 112
    mov dl, P_YELLOW
    mov dh, 2
    cmp word ptr [failed_sel], 1
    je .Lsfs_select
    mov dl, P_GREY
.Lsfs_select:
    call gfx_text_c
    mov si, offset str_failed_hint
    mov cx, 151
    mov dl, P_WHITE
    mov dh, 1
    call gfx_text_c
    call plat_present
.Lsfs_key:
    call wait_key
    cmp ax, K_UP
    je .Lsfs_up
    cmp ax, K_DOWN
    je .Lsfs_down
    cmp ax, 119
    je .Lsfs_up
    cmp ax, 115
    je .Lsfs_down
    cmp ax, K_ENTER
    je .Lsfs_choose
    jmp .Lsfs_key
.Lsfs_up:
    mov word ptr [failed_sel], 0
    jmp .Lsfs_draw
.Lsfs_down:
    mov word ptr [failed_sel], 1
    jmp .Lsfs_draw
.Lsfs_choose:
    call plat_flush_keys
    mov ax, word ptr [failed_sel]
    ret

# level_completed_screen: AX = 0 next level, AX = 1 stage selection.
level_completed_screen:
    mov word ptr [failed_sel], 0
.Llcs_draw:
    mov al, P_BLACK
    call gfx_clear_all
    # Small single-scale lettering with generous vertical spacing. The old
    # scale-2 heading/options overlapped on the 320x200 DOS screen.
    mov si, offset str_clear
    mov cx, 42
    mov dl, P_YELLOW
    mov dh, 1
    call gfx_text_c
    mov si, offset str_next_option
    mov cx, 82
    mov dl, P_YELLOW
    mov dh, 1
    cmp word ptr [failed_sel], 0
    je .Llcs_next
    mov dl, P_GREY
.Llcs_next:
    call gfx_text_c
    mov si, offset str_select_option
    mov cx, 106
    mov dl, P_YELLOW
    mov dh, 1
    cmp word ptr [failed_sel], 1
    je .Llcs_select
    mov dl, P_GREY
.Llcs_select:
    call gfx_text_c
    mov si, offset str_failed_hint
    mov cx, 151
    mov dl, P_WHITE
    mov dh, 1
    call gfx_text_c
    call plat_present
.Llcs_key:
    call wait_key
    cmp ax, K_UP
    je .Llcs_up
    cmp ax, K_DOWN
    je .Llcs_down
    cmp ax, 119
    je .Llcs_up
    cmp ax, 115
    je .Llcs_down
    cmp ax, K_ENTER
    je .Llcs_choose
    jmp .Llcs_key
.Llcs_up:
    mov word ptr [failed_sel], 0
    jmp .Llcs_draw
.Llcs_down:
    mov word ptr [failed_sel], 1
    jmp .Llcs_draw
.Llcs_choose:
    call plat_flush_keys
    mov ax, word ptr [failed_sel]
    ret

# pause_frames: CX = number of frames to keep drawing
pause_frames:
    mov word ptr [pf_n], cx
.Lpz1:
    cmp word ptr [pf_n], 0
    jle .Lpz2
    dec word ptr [pf_n]
    call draw_world
    call tick
    cmp word ptr [g_msg_t], 0
    jle .Lpz1
    dec word ptr [g_msg_t]
    jmp .Lpz1
.Lpz2:
    ret

# slide_transition: old room slides out, [next_room] slides in. BX=dirx, CX=diry
slide_transition:
    mov word ptr [st_dx], bx
    mov word ptr [st_dy], cx
    mov ax, VIEW_H
    test bx, bx
    jz .Lst_t
    mov ax, VIEW_W
.Lst_t:
    mov word ptr [st_total], ax
    mov ax, word ptr [next_room+R_SX]
    shl ax, 4
    mov word ptr [st_nsx], ax
    mov ax, word ptr [next_room+R_SY]
    shl ax, 4
    mov word ptr [st_nsy], ax
    mov ax, word ptr [st_nsx]
    add ax, 8
    sub ax, VIEW_W/2
    mov bx, word ptr [next_room+R_W]
    shl bx, 4
    mov cx, VIEW_W
    call clamp_cam
    mov word ptr [st_ncx], ax
    mov ax, word ptr [st_nsy]
    add ax, 8
    sub ax, VIEW_H/2
    mov bx, word ptr [next_room+R_H]
    shl bx, 4
    mov cx, VIEW_H
    call clamp_cam
    mov word ptr [st_ncy], ax
    mov word ptr [st_p], 8
.Lst_loop:
    mov ax, word ptr [st_total]
    add ax, 7
    cmp word ptr [st_p], ax          # p <= total + step - 1
    jg .Lst_done
    mov ax, word ptr [st_p]
    cmp ax, word ptr [st_total]
    jle .Lst_q
    mov ax, word ptr [st_total]
.Lst_q:
    mov word ptr [st_q], ax
    mov ax, word ptr [st_dx]         # oox = -cam_x - dirx*q
    imul ax, word ptr [st_q]
    mov bx, word ptr [cam_x]
    neg bx
    sub bx, ax
    mov word ptr [st_oox], bx
    mov ax, word ptr [st_dy]
    imul ax, word ptr [st_q]
    mov bx, word ptr [cam_y]
    neg bx
    sub bx, ax
    mov word ptr [st_ooy], bx
    mov ax, word ptr [st_total]      # nox = -ncx + dirx*(total-q)
    sub ax, word ptr [st_q]
    mov bx, ax
    imul ax, word ptr [st_dx]
    sub ax, word ptr [st_ncx]
    mov word ptr [st_nox], ax
    imul bx, word ptr [st_dy]
    sub bx, word ptr [st_ncy]
    mov word ptr [st_noy], bx
    mov al, P_BLACK
    call gfx_clear_all
    mov si, offset room
    mov bx, word ptr [st_oox]
    mov cx, word ptr [st_ooy]
    xor dl, dl
    call gfx_room
    mov bx, word ptr [g_px]
    shl bx, 4
    add bx, word ptr [st_oox]
    mov cx, word ptr [g_py]
    shl cx, 4
    add cx, word ptr [st_ooy]
    call gfx_player
    mov si, offset next_room
    mov bx, word ptr [st_nox]
    mov cx, word ptr [st_noy]
    mov dl, 1
    call gfx_room
    mov bx, word ptr [st_nsx]
    add bx, word ptr [st_nox]
    mov cx, word ptr [st_nsy]
    add cx, word ptr [st_noy]
    call gfx_player
    call gfx_hud
    call tick
    add word ptr [st_p], 8
    jmp .Lst_loop
.Lst_done:
    mov ax, word ptr [st_ncx]
    mov word ptr [cam_x], ax
    mov ax, word ptr [st_ncy]
    mov word ptr [cam_y], ax
    ret

# play: the in-game loop.  Returns when the player goes back to the menu / dies in endless mode
play:
    call plat_flush_keys
    call setup_room_camera
    call invalidate
.Lp_loop:
    mov word ptr [p_ev], 0
    cmp word ptr [g_mode], MODE_ENDLESS
    jne .Lp_lava_done
    inc word ptr [g_lava_clock]
    cmp word ptr [g_lava_clock], 70
    jl .Lp_lava_check
    mov word ptr [g_lava_clock], 0
    dec word ptr [g_lava_y]
    call invalidate
.Lp_lava_check:
    mov ax, word ptr [g_lava_y]
    cmp ax, word ptr [room+R_H]
    jge .Lp_lava_done
    cmp ax, 0
    jl .Lp_lava_done
    cmp word ptr [g_py], ax
    jl .Lp_lava_done
    mov word ptr [p_ev], EV_DEAD
    jmp .Lp_nodash
.Lp_lava_done:
    call game_dashing
    jnz .Lp_dash
    call plat_key                    # idle: read one command, drop stacked key repeats
    mov word ptr [p_k], ax
.Lp_drain:
    call plat_key
    test ax, ax
    jnz .Lp_drain
    cmp word ptr [p_k], 0
    je .Lp_dash
    mov ax, word ptr [p_k]
    call seed_mix
    mov ax, word ptr [p_k]
    call is_up
    test dl, dl
    jz .Lp1
    xor ax, ax
    mov bx, -1
    call game_dash
    jmp .Lp_dash
.Lp1:
    mov ax, word ptr [p_k]
    call is_down
    test dl, dl
    jz .Lp2
    xor ax, ax
    mov bx, 1
    call game_dash
    jmp .Lp_dash
.Lp2:
    mov ax, word ptr [p_k]
    call is_left
    test dl, dl
    jz .Lp3
    mov ax, -1
    xor bx, bx
    call game_dash
    jmp .Lp_dash
.Lp3:
    mov ax, word ptr [p_k]
    call is_right
    test dl, dl
    jz .Lp4
    mov ax, 1
    xor bx, bx
    call game_dash
    jmp .Lp_dash
.Lp4:
    mov ax, word ptr [p_k]
    cmp ax, 98                       # 'b'
    je .Lp_bomb
    cmp ax, 66
    jne .Lp5
.Lp_bomb:
    call game_bomb
    call invalidate
    jmp .Lp_dash
.Lp5:
    cmp ax, 114                      # 'r'
    je .Lp_restart
    cmp ax, 82
    je .Lp_restart
    cmp ax, 96                       # '`'
    jne .Lp6
.Lp_restart:
    call game_restart
    call invalidate
    jmp .Lp_dash
.Lp6:
    cmp ax, 104                      # 'h'
    je .Lp_help
    cmp ax, 72
    je .Lp_help
    cmp ax, K_TAB
    jne .Lp7
.Lp_help:
    call help_screen
    call plat_flush_keys
    call invalidate
    jmp .Lp_dash
.Lp7:
    cmp ax, 109                      # 'm'
    je .Lp_menu
    cmp ax, 77
    je .Lp_menu
    cmp ax, K_ESC
    jne .Lp_dash
.Lp_menu:
    ret
.Lp_dash:
    call game_dashing
    jz .Lp_nodash
    mov bx, word ptr [speed_idx]
    shl bx, 1
    mov ax, word ptr [speed_px+bx]
    call game_tick
    mov word ptr [p_ev], ax
.Lp_nodash:
    cmp word ptr [g_msg_t], 0
    jle .Lp8
    dec word ptr [g_msg_t]
.Lp8:
    call cam_target
    mov ax, word ptr [cam_x]
    mov bx, word ptr [cam_tx]
    call ease
    mov word ptr [cam_x], ax
    mov ax, word ptr [cam_y]
    mov bx, word ptr [cam_ty]
    call ease
    mov word ptr [cam_y], ax
    call draw_world
    call tick
    test word ptr [p_ev], EV_DEAD
    jz .Lp_noded
    mov word ptr [g_msg], offset str_stage_failed
    mov word ptr [g_msg_t], 0
    call invalidate
    cmp word ptr [g_mode], MODE_ENDLESS
    jne .Lp_failed_normal
    mov cx, 20
    call pause_frames
    mov ax, word ptr [g_depth]
    dec ax
    mov word ptr [e_rooms], ax
    mov ax, word ptr [g_coins]
    mov word ptr [e_coins], ax
    mov si, offset str_gameover
    call end_screen
    ret
.Lp_failed_normal:
    call stage_failed_screen
    cmp ax, 1
    jne .Lp_restart_lvl
    mov word ptr [g_fail_select], 1
    ret
.Lp_restart_lvl:
    call game_restart
    call setup_room_camera
    call invalidate
    call plat_flush_keys
.Lp_noded:
    test word ptr [p_ev], EV_CLEAR
    jz .Lp_loop
    cmp word ptr [g_mode], MODE_NORMAL
    jne .Lp_clear_auto
    call level_completed_screen
    cmp ax, 1
    jne .Lp_clear_auto
    mov word ptr [g_fail_select], 1
    ret
.Lp_clear_auto:
    mov word ptr [g_msg], offset str_clear
    mov word ptr [g_msg_t], 100
    call draw_world
    call tick
    mov word ptr [p_dirx], 0
    mov word ptr [p_diry], -1
    cmp word ptr [room+R_EX], 0
    jne .Lpc1
    mov word ptr [p_dirx], -1
    mov word ptr [p_diry], 0
    jmp .Lpc2
.Lpc1:
    mov ax, word ptr [room+R_W]
    dec ax
    cmp word ptr [room+R_EX], ax
    jne .Lpc2
    mov word ptr [p_dirx], 1
    mov word ptr [p_diry], 0
.Lpc2:
    cmp word ptr [g_mode], MODE_NORMAL
    jne .Lpc_endless
    inc word ptr [g_level]
    cmp word ptr [g_level], NUM_LEVELS
    jl .Lpc_load
    mov word ptr [e_rooms], -1
    mov ax, word ptr [g_coins]
    mov word ptr [e_coins], ax
    mov si, offset str_win
    call end_screen
    ret
.Lpc_load:
    mov ax, word ptr [g_level]
    mov di, offset next_room
    call room_load_level
    jmp .Lpc_go
.Lpc_endless:
    inc word ptr [g_depth]
    mov ax, word ptr [g_depth]
    mov di, offset next_room
    call gen_room                    # hidden inside the pause below
.Lpc_go:
    mov cx, 25
    call pause_frames
    mov bx, word ptr [p_dirx]
    mov cx, word ptr [p_diry]
    call slide_transition
    mov si, offset next_room
    mov di, offset room
    call copy_room
    cmp word ptr [g_mode], MODE_ENDLESS
    jne .Lp_enter_room
    mov ax, word ptr [room+R_H]
    add ax, 4
    mov word ptr [g_lava_y], ax
    mov word ptr [g_lava_clock], 0
.Lp_enter_room:
    call game_enter_room
    call setup_room_camera
    call invalidate
    call plat_flush_keys
    jmp .Lp_loop

# start_game: AX = mode
start_game:
    mov word ptr [g_mode], ax
    mov word ptr [g_coins], 0
    mov word ptr [g_depth], 1
    cmp ax, MODE_NORMAL
    jne .Lsg1
    mov ax, word ptr [g_level]
    mov di, offset room
    call room_load_level
    jmp .Lsg2
.Lsg1:
    mov word ptr [g_level], 0
    mov ax, 1
    mov di, offset room
    call gen_room
.Lsg2:
    # Endless mode: lava begins below the room and climbs toward the player.
    cmp word ptr [g_mode], MODE_ENDLESS
    jne .Lsg_lava_done
    mov ax, word ptr [room+R_H]
    add ax, 4
    mov word ptr [g_lava_y], ax
    mov word ptr [g_lava_clock], 0
.Lsg_lava_done:
    call game_enter_room
    jmp play

    .ifdef SELFTEST
# test build: generate one room with a fixed seed/depth, then exit (memory is dumped by the emulator)
game_main:
    mov dword ptr [seed], TSEED
    mov ax, TDEPTH
    mov di, offset room
    call gen_room
    ret
    .else
game_main:
    mov dword ptr [seed], 44257
    mov word ptr [speed_idx], 1
    call plat_init
.Lgm_loop:
    cmp word ptr [g_fail_select], 0
    je .Lgm_menu
    mov word ptr [g_fail_select], 0
    call stage_select
    cmp ax, 0
    jl .Lgm_loop
    mov word ptr [g_level], ax
    mov ax, MODE_NORMAL
    call start_game
    jmp .Lgm_loop
.Lgm_menu:
    call menu_screen
    cmp ax, 0
    jl .Lgm_quit
    jne .Lgm1
    call stage_select
    cmp ax, 0
    jl .Lgm_loop
    mov word ptr [g_level], ax
    mov ax, MODE_NORMAL
    call start_game
    jmp .Lgm_loop
.Lgm1:
    cmp ax, 1
    jne .Lgm2
    mov ax, MODE_ENDLESS
    call start_game
    jmp .Lgm_loop
.Lgm2:
    call settings_screen
    jmp .Lgm_loop
.Lgm_quit:
    call plat_quit
    ret
    .endif
