# ============================================================================
#  Graphics: clipped tile blits, text, auto-tiled walls, spiked traps, HUD.
#  All drawing goes to the back buffer (ES).  Sets gfx_dirty so the main loop
#  knows a copy to the screen is needed.
# ============================================================================
    .text
clip_all:
    mov word ptr [clip_x1], SCR_W
    mov word ptr [clip_y1], SCR_H
    ret
clip_view:
    mov word ptr [clip_x1], VIEW_W
    mov word ptr [clip_y1], VIEW_H
    ret

# gfx_clear_all / gfx_clear_view: AL = colour
gfx_clear_all:
    mov byte ptr [gfx_dirty], 1
    mov ah, al
    mov dx, ax
    shl eax, 16
    mov ax, dx
    xor di, di
    mov cx, 16000
    cld
    rep stosd
    ret
gfx_clear_view:
    mov byte ptr [gfx_dirty], 1
    mov ah, al
    mov dx, ax
    shl eax, 16
    mov ax, dx
    xor di, di
    mov cx, VIEW_H*80
    cld
    rep stosd
    ret

# gfx_rect: BX=x CX=y SI=w DI=h DL=colour   (clipped to the screen)
gfx_rect:
    mov byte ptr [gfx_dirty], 1
    mov byte ptr [r_col], dl
    test bx, bx
    jns .Lr1
    add si, bx
    xor bx, bx
.Lr1:
    mov ax, bx
    add ax, si
    cmp ax, SCR_W
    jle .Lr2
    mov si, SCR_W
    sub si, bx
.Lr2:
    test si, si
    jle .Lr_ret
    mov word ptr [r_w], si
    mov word ptr [r_x], bx
    mov word ptr [r_y], cx
    mov word ptr [r_rows], di
.Lr_row:
    cmp word ptr [r_rows], 0
    jle .Lr_ret
    mov ax, word ptr [r_y]
    test ax, ax
    js .Lr_next
    cmp ax, SCR_H
    jge .Lr_next
    imul di, ax, SCR_W
    add di, word ptr [r_x]
    mov cx, word ptr [r_w]
    mov al, byte ptr [r_col]
    cld
    rep stosb
.Lr_next:
    inc word ptr [r_y]
    dec word ptr [r_rows]
    jmp .Lr_row
.Lr_ret:
    ret

# blit: draw a 16x16 tile.  SI=tile data, BX=x, CX=y, DL=1 if colour 255 is transparent
#       clipped to [0,clip_x1) x [0,clip_y1)
blit:
    mov byte ptr [gfx_dirty], 1
    mov word ptr [b_x], bx
    mov word ptr [b_y], cx
    mov byte ptr [b_key], dl
    mov word ptr [b_src], si
    mov word ptr [b_x0], 0
    mov word ptr [b_xe], 16
    test bx, bx
    jns .Lb1
    mov ax, bx
    neg ax
    mov word ptr [b_x0], ax
.Lb1:
    mov ax, word ptr [b_x]
    add ax, 16
    cmp ax, word ptr [clip_x1]
    jle .Lb2
    mov ax, word ptr [clip_x1]
    sub ax, word ptr [b_x]
    mov word ptr [b_xe], ax
.Lb2:
    mov ax, word ptr [b_x0]
    cmp ax, word ptr [b_xe]
    jge .Lb_ret
    mov word ptr [b_row], 0
.Lb_row:
    mov ax, word ptr [b_y]
    add ax, word ptr [b_row]
    js .Lb_next
    cmp ax, word ptr [clip_y1]
    jge .Lb_next
    imul di, ax, SCR_W
    add di, word ptr [b_x]
    mov si, word ptr [b_row]
    shl si, 4
    add si, word ptr [b_src]
    cmp byte ptr [b_key], 0
    jne .Lb_keyed
    add si, word ptr [b_x0]
    add di, word ptr [b_x0]
    mov cx, word ptr [b_xe]
    sub cx, word ptr [b_x0]
    cld
    rep movsb
    jmp .Lb_next
.Lb_keyed:
    mov bx, word ptr [b_x0]          # i
.Lk1:
    cmp bx, word ptr [b_xe]
    jge .Lb_next
    cmp byte ptr [si+bx], 255
    jne .Lk2
    inc bx
    jmp .Lk1
.Lk2:
    mov bp, bx                       # j = end of the run of opaque pixels
.Lk3:
    cmp bp, word ptr [b_xe]
    jge .Lk4
    cmp byte ptr [si+bp], 255
    je .Lk4
    inc bp
    jmp .Lk3
.Lk4:
    push si
    push di
    add si, bx
    add di, bx
    mov cx, bp
    sub cx, bx
    cld
    rep movsb
    pop di
    pop si
    mov bx, bp
    jmp .Lk1
.Lb_next:
    inc word ptr [b_row]
    cmp word ptr [b_row], 16
    jl .Lb_row
.Lb_ret:
    ret

# tile_ptr: AX = tile index -> SI = address of its 256 bytes
tile_ptr:
    mov si, ax
    shl si, 8
    add si, offset tile_data
    ret

# ---------------------------------------------------------------- text (5x7 font, 6x8 cell)
# gfx_glyph: AL=char BX=x CX=y DL=colour DH=scale
gfx_glyph:
    mov byte ptr [gfx_dirty], 1
    mov word ptr [gl_x], bx
    mov word ptr [gl_y], cx
    mov byte ptr [gl_col], dl
    movzx dx, dh
    mov word ptr [gl_sc], dx
    movzx ax, al
    cmp ax, 97
    jb .Lg1
    cmp ax, 122
    ja .Lg1
    sub ax, 32
.Lg1:
    cmp ax, 32
    jb .Lg_bad
    cmp ax, 95
    jbe .Lg_ok
.Lg_bad:
    mov ax, 32
.Lg_ok:
    sub ax, 32
    imul ax, ax, 7
    add ax, offset font5x7
    mov word ptr [gl_ptr], ax
    mov word ptr [gl_r], 0
.Lg_row:
    mov si, word ptr [gl_ptr]
    add si, word ptr [gl_r]
    movzx ax, byte ptr [si]
    mov word ptr [gl_bits], ax
    mov word ptr [gl_c], 0
.Lg_c:
    cmp word ptr [gl_c], 5
    jge .Lg_nextrow
    mov cx, 4
    sub cx, word ptr [gl_c]
    mov ax, word ptr [gl_bits]
    shr ax, cl
    test al, 1
    jz .Lg_skip
    mov ax, word ptr [gl_c]
    mov word ptr [gl_e], ax
.Lg_e:
    cmp word ptr [gl_e], 5
    jge .Lg_edone
    mov cx, 4
    sub cx, word ptr [gl_e]
    mov ax, word ptr [gl_bits]
    shr ax, cl
    test al, 1
    jz .Lg_edone
    inc word ptr [gl_e]
    jmp .Lg_e
.Lg_edone:
    mov word ptr [gl_k], 0
.Lg_k:
    mov ax, word ptr [gl_k]
    cmp ax, word ptr [gl_sc]
    jge .Lg_kdone
    mov ax, word ptr [gl_r]
    imul ax, word ptr [gl_sc]
    add ax, word ptr [gl_y]
    add ax, word ptr [gl_k]          # yy
    test ax, ax
    js .Lg_kskip
    cmp ax, SCR_H
    jge .Lg_kskip
    mov dx, word ptr [gl_c]
    imul dx, word ptr [gl_sc]
    add dx, word ptr [gl_x]          # xs
    js .Lg_kskip
    mov bx, word ptr [gl_e]
    imul bx, word ptr [gl_sc]
    add bx, word ptr [gl_x]
    cmp bx, SCR_W
    jg .Lg_kskip
    imul di, ax, SCR_W
    add di, dx
    mov cx, word ptr [gl_e]
    sub cx, word ptr [gl_c]
    imul cx, word ptr [gl_sc]
    mov al, byte ptr [gl_col]
    cld
    rep stosb
.Lg_kskip:
    inc word ptr [gl_k]
    jmp .Lg_k
.Lg_kdone:
    mov ax, word ptr [gl_e]
    mov word ptr [gl_c], ax
    jmp .Lg_c
.Lg_skip:
    inc word ptr [gl_c]
    jmp .Lg_c
.Lg_nextrow:
    inc word ptr [gl_r]
    cmp word ptr [gl_r], 7
    jl .Lg_row
    ret

# gfx_text: SI=zero terminated string, BX=x CX=y DL=colour DH=scale
gfx_text:
    mov word ptr [tx_x], bx
    mov word ptr [tx_y], cx
    mov byte ptr [tx_col], dl
    mov byte ptr [tx_sc], dh
    mov word ptr [tx_ptr], si
.Ltx_loop:
    mov si, word ptr [tx_ptr]
    mov al, byte ptr [si]
    test al, al
    jz .Ltx_ret
    mov bx, word ptr [tx_x]
    mov cx, word ptr [tx_y]
    mov dl, byte ptr [tx_col]
    mov dh, byte ptr [tx_sc]
    call gfx_glyph
    inc word ptr [tx_ptr]
    movzx ax, byte ptr [tx_sc]
    imul ax, ax, 6
    add word ptr [tx_x], ax
    jmp .Ltx_loop
.Ltx_ret:
    ret

# gfx_text_w: SI=string, DH=scale -> AX = pixel width
gfx_text_w:
    xor ax, ax
    mov bx, si
.Ltw1:
    cmp byte ptr [bx], 0
    je .Ltw2
    inc ax
    inc bx
    jmp .Ltw1
.Ltw2:
    imul ax, ax, 6
    movzx dx, dh
    imul ax, dx
    ret

# gfx_text_c: centred text.  SI=string CX=y DL=colour DH=scale
gfx_text_c:
    push dx
    call gfx_text_w
    mov bx, SCR_W
    sub bx, ax
    sar bx, 1
    pop dx
    jmp gfx_text

# gfx_num: AX=number, DI=digits (1..7), BX=x CX=y DL=colour DH=scale
gfx_num:
    push bx
    push cx
    push dx
    mov bx, di
    mov byte ptr [num_buf+bx], 0
.Ln1:
    test bx, bx
    jz .Ln2
    dec bx
    xor dx, dx
    mov cx, 10
    div cx
    add dl, 48
    mov byte ptr [num_buf+bx], dl
    jmp .Ln1
.Ln2:
    pop dx
    pop cx
    pop bx
    mov si, offset num_buf
    jmp gfx_text

# ---------------------------------------------------------------- walls and traps
# solid: BX=room, AX=x, CX=y (signed) -> AL=1 if the cell counts as wall (outside = wall)
solid:
    test ax, ax
    js .Ls1
    test cx, cx
    js .Ls1
    cmp ax, word ptr [bx+R_W]
    jge .Ls1
    cmp cx, word ptr [bx+R_H]
    jge .Ls1
    push si
    mov si, cx
    imul si, word ptr [bx+R_W]
    add si, ax
    mov al, byte ptr [bx+si+R_C]
    pop si
    cmp al, T_WALL
    je .Ls1
    cmp al, T_TRAP
    je .Ls1
    cmp al, T_BREAK
    je .Ls1
    cmp al, T_TOGGLE
    je .Ls1
    xor al, al
    ret
.Ls1:
    mov al, 1
    ret

# open_mask: BX=room, AX=x, CX=y -> AL = bit0 up open, bit1 down, bit2 left, bit3 right
open_mask:
    mov word ptr [om_x], ax
    mov word ptr [om_y], cx
    xor dh, dh
    dec cx                           # up
    call solid
    test al, al
    jnz .Lom1
    or dh, 1
.Lom1:
    mov ax, word ptr [om_x]
    mov cx, word ptr [om_y]
    inc cx                           # down
    call solid
    test al, al
    jnz .Lom2
    or dh, 2
.Lom2:
    mov ax, word ptr [om_x]
    dec ax                           # left
    mov cx, word ptr [om_y]
    call solid
    test al, al
    jnz .Lom3
    or dh, 4
.Lom3:
    mov ax, word ptr [om_x]
    inc ax                           # right
    mov cx, word ptr [om_y]
    call solid
    test al, al
    jnz .Lom4
    or dh, 8
.Lom4:
    mov al, dh
    ret

# build_trap: AL = open mask. Lethal wall: red block with saw teeth on every open side.
build_trap:
    mov byte ptr [bt_mask], al
    push es
    push ds
    pop es
    mov di, offset trap_tile
    mov cx, 256
    mov al, P_TRAP
    cld
    rep stosb
    pop es
    xor bx, bx                       # rim
.Lbt_rim:
    mov al, byte ptr [bt_mask]
    test al, 1
    jz .Lbt_r1
    mov byte ptr [trap_tile+bx], P_TRAP_SP
.Lbt_r1:
    test al, 2
    jz .Lbt_r2
    mov byte ptr [trap_tile+240+bx], P_TRAP_SP
.Lbt_r2:
    mov si, bx
    shl si, 4
    test al, 4
    jz .Lbt_r3
    mov byte ptr [trap_tile+si], P_TRAP_SP
.Lbt_r3:
    test al, 8
    jz .Lbt_r4
    mov byte ptr [trap_tile+si+15], P_TRAP_SP
.Lbt_r4:
    inc bx
    cmp bx, 16
    jl .Lbt_rim
    xor bx, bx                       # teeth, bx = x
.Lbt_x:
    mov ax, bx
    and ax, 7                        # t
    cmp ax, 4
    jge .Lbt_h2
    inc ax                           # h = t+1
    jmp .Lbt_h3
.Lbt_h2:
    mov cx, 8
    sub cx, ax
    mov ax, cx                       # h = 8-t
.Lbt_h3:
    mov word ptr [bt_h], ax
    xor si, si                       # si = d
.Lbt_d:
    mov ax, 4
    sub ax, word ptr [bt_h]          # lit = d >= 4-h
    mov dl, P_FLOOR
    cmp si, ax
    jl .Lbt_c
    mov dl, P_TRAP_HI
.Lbt_c:
    mov al, byte ptr [bt_mask]
    test al, 1
    jz .Lbt_m2
    mov di, si
    shl di, 4
    mov byte ptr [trap_tile+di+bx], dl        # [d*16 + x]
.Lbt_m2:
    test al, 2
    jz .Lbt_m3
    mov di, 15
    sub di, si
    shl di, 4
    mov byte ptr [trap_tile+di+bx], dl        # [(15-d)*16 + x]
.Lbt_m3:
    test al, 4
    jz .Lbt_m4
    mov di, bx
    shl di, 4
    add di, si
    mov byte ptr [trap_tile+di], dl           # [x*16 + d]
.Lbt_m4:
    test al, 8
    jz .Lbt_m5
    mov di, bx
    shl di, 4
    add di, 15
    sub di, si
    mov byte ptr [trap_tile+di], dl           # [x*16 + 15 - d]
.Lbt_m5:
    inc si
    cmp si, 4
    jl .Lbt_d
    inc bx
    cmp bx, 16
    jl .Lbt_x
    mov byte ptr [trap_tile+7*16+7], P_RED_HI
    mov byte ptr [trap_tile+7*16+8], P_RED_HI
    mov byte ptr [trap_tile+8*16+7], P_RED_HI
    mov byte ptr [trap_tile+8*16+8], P_RED_HI
    ret

# tile_for: BX=room, AX=x, CX=y -> SI = tile bitmap for that cell
tile_for:
    mov word ptr [tf_x], ax
    mov word ptr [tf_y], cx
    mov si, cx
    imul si, word ptr [bx+R_W]
    add si, ax
    mov dl, byte ptr [bx+si+R_C]
    cmp dl, T_WALL
    jne .Ltf1
    call open_mask
    movzx ax, al
    add ax, TI_WALL0
    jmp tile_ptr
.Ltf1:
    cmp dl, T_TRAP
    jne .Ltf2
    call open_mask
    call build_trap
    mov si, offset trap_tile
    ret
.Ltf2:
    cmp dl, T_TRAIL
    jne .Ltftrail1
    mov si, offset trail_tile
    ret
.Ltftrail1:
    cmp dl, T_COIN
    jne .Ltf3
    mov si, offset pellet_tile
    ret
.Ltf3:
    mov bx, offset tf_table
.Ltf4:
    mov al, byte ptr [bx]
    test al, al
    jz .Ltf_floor
    cmp al, dl
    je .Ltf_found
    add bx, 2
    jmp .Ltf4
.Ltf_found:
    movzx ax, byte ptr [bx+1]
    jmp tile_ptr
.Ltf_floor:
    mov ax, TI_FLOOR
    jmp tile_ptr
tf_table:                            # character -> tile index
    .byte T_BREAK, TI_BREAK
    .byte T_TOGGLE, TI_TOGGLE
    .byte T_GATE, TI_GATE
    .byte T_DOOR, TI_DOOR
    .byte T_PLATE, TI_PLATE
    .byte T_KEY, TI_KEY
    .byte T_LEVER, TI_LEVER
    .byte T_BOMB, TI_BOMB
    .byte T_SHIELD, TI_SHIELD
    .byte T_CRATE, TI_CRATE
    .byte 0, 0

# gfx_sprite_tile: AL=tile character ('z' = player), BX=x, CX=y.  Full-screen clip.
gfx_sprite_tile:
    push bx
    push cx
    mov dl, al
    call clip_all
    cmp dl, 122                      # 'z'
    jne .Lst1
    pop cx
    pop bx
    push bx
    push cx
    mov ax, TI_FLOOR
    call tile_ptr
    xor dl, dl
    call blit
    pop cx
    pop bx
    mov si, offset player_data
    mov dl, 1
    jmp blit
.Lst1:
    cmp dl, T_COIN
    jne .Lst2
    mov ax, TI_COIN0
    jmp .Lst_do
.Lst2:
    cmp dl, T_WALL
    jne .Lst3
    mov ax, TI_WALL0
    jmp .Lst_do
.Lst3:
    cmp dl, T_TRAP
    jne .Lst4
    mov al, 15
    call build_trap
    mov si, offset trap_tile
    pop cx
    pop bx
    xor dl, dl
    jmp blit
.Lst4:
    mov bx, offset tf_table
.Lst5:
    mov al, byte ptr [bx]
    test al, al
    jz .Lst_floor
    cmp al, dl
    je .Lst6
    add bx, 2
    jmp .Lst5
.Lst6:
    movzx ax, byte ptr [bx+1]
    jmp .Lst_do
.Lst_floor:
    mov ax, TI_FLOOR
.Lst_do:
    call tile_ptr
    pop cx
    pop bx
    xor dl, dl
    jmp blit

# gfx_room: draw a room with its top-left corner at (BX,CX) in the view window.
#           SI=room, DL = 1 to draw the animated exit marker.  Uses the global 'frame'.
gfx_room:
    mov word ptr [gr_room], si
    mov word ptr [gr_ox], bx
    mov word ptr [gr_oy], cx
    mov byte ptr [gr_exit], dl
    xor ax, ax                       # tx0 = ox>0 ? 0 : (-ox)/16
    test bx, bx
    jg .Lgr1
    mov ax, bx
    neg ax
    sar ax, 4
.Lgr1:
    mov word ptr [gr_tx0], ax
    xor ax, ax
    test cx, cx
    jg .Lgr2
    mov ax, cx
    neg ax
    sar ax, 4
.Lgr2:
    mov word ptr [gr_ty0], ax
    mov ax, VIEW_W+15                # tx1 = (VIEW_W - ox + 15)/16
    sub ax, bx
    sar ax, 4
    mov si, word ptr [gr_room]
    cmp ax, word ptr [si+R_W]
    jle .Lgr3
    mov ax, word ptr [si+R_W]
.Lgr3:
    mov word ptr [gr_tx1], ax
    mov ax, VIEW_H+15
    sub ax, cx
    sar ax, 4
    cmp ax, word ptr [si+R_H]
    jle .Lgr4
    mov ax, word ptr [si+R_H]
.Lgr4:
    mov word ptr [gr_ty1], ax
    call clip_view
    mov ax, word ptr [gr_ty0]
    mov word ptr [gr_ty], ax
.Lgr_y:
    mov ax, word ptr [gr_ty]
    cmp ax, word ptr [gr_ty1]
    jge .Lgr_done
    mov ax, word ptr [gr_tx0]
    mov word ptr [gr_tx], ax
.Lgr_x:
    mov ax, word ptr [gr_tx]
    cmp ax, word ptr [gr_tx1]
    jge .Lgr_ynext
    mov bx, ax                       # sx = ox + tx*16
    shl bx, 4
    add bx, word ptr [gr_ox]
    mov cx, word ptr [gr_ty]         # sy = oy + ty*16
    shl cx, 4
    add cx, word ptr [gr_oy]
    lea ax, [bx+16]
    test ax, ax
    jle .Lgr_xnext
    mov ax, cx
    add ax, 16
    jle .Lgr_xnext
    cmp bx, VIEW_W
    jge .Lgr_xnext
    cmp cx, VIEW_H
    jge .Lgr_xnext
    mov word ptr [gr_sx], bx
    mov word ptr [gr_sy], cx
    cmp byte ptr [gr_exit], 0
    je .Lgr_normal
    mov si, word ptr [gr_room]
    mov ax, word ptr [gr_tx]
    cmp ax, word ptr [si+R_EX]
    jne .Lgr_normal
    mov ax, word ptr [gr_ty]
    cmp ax, word ptr [si+R_EY]
    jne .Lgr_normal
    mov ax, word ptr [frame]         # animated exit marker
    xor dx, dx
    mov cx, 14
    div cx
    and ax, 1
    add ax, TI_EXIT0
    call tile_ptr
    jmp .Lgr_blit
.Lgr_normal:
    mov bx, word ptr [gr_room]
    mov ax, word ptr [gr_tx]
    mov cx, word ptr [gr_ty]
    call tile_for
.Lgr_blit:
    mov bx, word ptr [gr_sx]
    mov cx, word ptr [gr_sy]
    xor dl, dl
    call blit
.Lgr_xnext:
    inc word ptr [gr_tx]
    jmp .Lgr_x
.Lgr_ynext:
    inc word ptr [gr_ty]
    jmp .Lgr_y
.Lgr_done:
    call clip_all
    ret

# gfx_player: BX=x CY=y in the view
gfx_player:
    call clip_view
    mov si, offset player_data
    mov dl, 1
    call blit
    jmp clip_all

# gfx_hud: draws the 32 pixel status bar from the game variables
gfx_hud:
    mov bx, 0
    mov cx, VIEW_H
    mov si, SCR_W
    mov di, 32
    mov dl, P_HUD
    call gfx_rect
    mov bx, 0
    mov cx, VIEW_H
    mov si, SCR_W
    mov di, 1
    mov dl, P_WALL_HI
    call gfx_rect
    mov al, T_COIN
    mov bx, 4
    mov cx, VIEW_H+3
    call gfx_sprite_tile
    mov ax, word ptr [g_coins]
    mov di, 3
    mov bx, 22
    mov cx, VIEW_H+6
    mov dl, P_YELLOW
    mov dh, 2
    call gfx_num
    mov al, T_KEY
    mov bx, 70
    mov cx, VIEW_H+3
    call gfx_sprite_tile
    mov ax, word ptr [g_keys]
    mov di, 1
    mov bx, 88
    mov cx, VIEW_H+6
    mov dl, P_SILVER
    mov dh, 2
    call gfx_num
    mov al, T_BOMB
    mov bx, 112
    mov cx, VIEW_H+3
    call gfx_sprite_tile
    mov ax, word ptr [g_bombs]
    mov di, 1
    mov bx, 130
    mov cx, VIEW_H+6
    mov dl, P_RED_HI
    mov dh, 2
    call gfx_num
    cmp word ptr [g_shield], 0
    je .Lh_noshield
    mov al, T_SHIELD
    mov bx, 154
    mov cx, VIEW_H+3
    call gfx_sprite_tile
    jmp .Lh_label
.Lh_noshield:
    mov si, offset str_dash
    mov bx, 160
    mov cx, VIEW_H+6
    mov dl, P_GREY
    mov dh, 2
    call gfx_text
.Lh_label:
    mov di, offset hud_label         # "ROOM n" or "LVL n"
    cmp word ptr [g_mode], MODE_ENDLESS
    jne .Lh_lvl
    mov si, offset str_room
    mov ax, word ptr [g_depth]
    jmp .Lh_copy
.Lh_lvl:
    mov si, offset str_lvl
    mov ax, word ptr [g_level]
    inc ax
.Lh_copy:
    mov cx, ax                       # value
.Lh_c1:
    mov al, byte ptr [si]
    test al, al
    jz .Lh_c2
    mov byte ptr [di], al
    inc si
    inc di
    jmp .Lh_c1
.Lh_c2:
    mov ax, cx                       # digits
    mov bx, 1
    cmp ax, 10
    jl .Lh_c3
    mov bx, 2
    cmp ax, 100
    jl .Lh_c3
    mov bx, 3
.Lh_c3:
    mov byte ptr [di+bx], 0
    mov ax, cx
.Lh_c4:
    test bx, bx
    jz .Lh_c5
    dec bx
    xor dx, dx
    mov cx, 10
    div cx
    add dl, 48
    mov byte ptr [di+bx], dl
    jmp .Lh_c4
.Lh_c5:
    mov si, offset hud_label
    mov dh, 2
    call gfx_text_w
    mov bx, SCR_W-4
    sub bx, ax
    mov cx, VIEW_H+6
    mov dl, P_WALL_HI
    mov dh, 2
    call gfx_text
    cmp word ptr [g_msg_t], 0
    jle .Lh_nomsg
    mov si, word ptr [g_msg]
    mov bx, 4
    mov cx, VIEW_H+22
    mov dl, P_WHITE
    mov dh, 1
    jmp gfx_text
.Lh_nomsg:
    mov si, offset str_hint
    mov bx, 4
    mov cx, VIEW_H+22
    mov dl, P_GREY
    mov dh, 1
    jmp gfx_text
