# ============================================================================
#  Endless-mode
# ============================================================================
    .equ MAXNX, 12
    .equ MAXNY, 16
    .equ EDGE_EXIT, 255
    .text

gfill:
    mov word ptr [f_x0], ax
    mov word ptr [f_y0], bx
    mov word ptr [f_x1], cx
    mov word ptr [f_y1], dx
    mov word ptr [f_c], di
    mov bp, word ptr [gp_r]
    mov ax, word ptr [f_y0]
    mov word ptr [f_y], ax
.Lgf_y:
    mov ax, word ptr [f_y]
    cmp ax, word ptr [f_y1]
    jg .Lgf_done
    mov si, ax
    imul si, word ptr [bp+R_W]
    add si, word ptr [f_x0]
    mov cx, word ptr [f_x1]
    sub cx, word ptr [f_x0]
    inc cx
    jle .Lgf_next
    mov al, byte ptr [f_c]
.Lgf_x:
    mov byte ptr [bp+si+R_C], al
    inc si
    loop .Lgf_x
.Lgf_next:
    inc word ptr [f_y]
    jmp .Lgf_y
.Lgf_done:
    ret

glink:
    imul ax, ax, 3
    inc ax
    imul bx, bx, 3
    inc bx
    imul cx, cx, 3
    inc cx
    imul dx, dx, 3
    inc dx
    cmp ax, cx
    jle .Lgl1
    xchg ax, cx
.Lgl1:
    cmp bx, dx
    jle .Lgl2
    xchg bx, dx
.Lgl2:
    mov di, T_FLOOR
    jmp gfill

# gfree8: all 8 neighbours are floor
gfree8:
    mov word ptr [fr_x], ax
    mov word ptr [fr_y], bx
    mov bp, word ptr [gp_r]
    mov cx, -1                       # dy
.Lfr_y:
    mov dx, -1                       # dx
.Lfr_x:
    mov si, word ptr [fr_y]
    add si, cx
    imul si, word ptr [bp+R_W]
    add si, word ptr [fr_x]
    add si, dx
    cmp byte ptr [bp+si+R_C], T_FLOOR
    jne .Lfr_no
    inc dx
    cmp dx, 1
    jle .Lfr_x
    inc cx
    cmp cx, 1
    jle .Lfr_y
    mov al, 1
    ret
.Lfr_no:
    xor al, al
    ret

# layout: build the walls/corridors of room.
layout:
    mov bp, word ptr [gp_r]
    mov ax, word ptr [l_nx]
    imul ax, ax, 3
    mov word ptr [l_w], ax           # W = 3*nx
    mov word ptr [bp+R_W], ax
    mov ax, word ptr [l_ny]
    imul ax, ax, 3
    mov word ptr [l_h], ax           # H = 3*ny
    mov word ptr [bp+R_H], ax
    xor ax, ax
    xor bx, bx
    mov cx, word ptr [l_w]
    dec cx
    mov dx, word ptr [l_h]
    dec dx
    mov di, T_WALL
    call gfill
    mov ax, word ptr [l_nx]          # hx = mirror ? (nx+1)/2 : nx ; xmax = mirror ? (W-1)/2 : W-2
    mov bx, word ptr [l_w]
    sub bx, 2
    cmp word ptr [l_mirror], 0
    je .Lly_h
    inc ax
    shr ax, 1
    mov bx, word ptr [l_w]
    dec bx
    shr bx, 1
.Lly_h:
    mov word ptr [l_hx], ax
    mov word ptr [l_xmax], bx
    mov di, offset seen              # clear the visited flags
    mov cx, MAXNX*MAXNY
    xor al, al
    push es
    push ds
    pop es
    cld
    rep stosb
    pop es
    mov word ptr [l_top], 0
    mov cx, word ptr [l_hx]
    call rnd
    mov bx, word ptr [l_ny]
    dec bx
    imul bx, bx, MAXNX
    add ax, bx
    mov word ptr [lstack], ax
    mov bx, ax
    mov byte ptr [seen+bx], 1
    xor dx, dx                       # carve the first node
    mov cx, MAXNX
    div cx                           # AX = cj, DX = ci
    imul dx, dx, 3
    inc dx
    imul ax, ax, 3
    inc ax
    mov bx, ax
    mov ax, dx
    mov cx, ax
    mov dx, bx
    mov di, T_FLOOR
    call gfill
.Lly_dfs:
    cmp word ptr [l_top], 0
    jl .Lly_braid
    mov bx, word ptr [l_top]
    shl bx, 1
    mov ax, word ptr [lstack+bx]
    xor dx, dx
    mov cx, MAXNX
    div cx
    mov word ptr [l_ci], dx
    mov word ptr [l_cj], ax
    mov word ptr [l_no], 0
    xor si, si                       # direction d
.Lly_dir:
    mov bx, si
    shl bx, 1
    mov ax, word ptr [l_ci]
    add ax, word ptr [bomb_ox+bx]    # ni
    mov cx, word ptr [l_cj]
    add cx, word ptr [bomb_oy+bx]    # nj
    test ax, ax
    js .Lly_dnext
    test cx, cx
    js .Lly_dnext
    cmp ax, word ptr [l_hx]
    jge .Lly_dnext
    cmp cx, word ptr [l_ny]
    jge .Lly_dnext
    imul cx, cx, MAXNX
    add cx, ax
    mov bx, cx
    cmp byte ptr [seen+bx], 0
    jne .Lly_dnext
    mov bx, word ptr [l_no]
    shl bx, 1
    mov word ptr [l_opt+bx], si
    inc word ptr [l_no]
.Lly_dnext:
    inc si
    cmp si, 4
    jl .Lly_dir
    cmp word ptr [l_no], 0
    jne .Lly_pick
    dec word ptr [l_top]
    jmp .Lly_dfs
.Lly_pick:
    mov cx, word ptr [l_no]
    call rnd
    mov bx, ax
    shl bx, 1
    mov si, word ptr [l_opt+bx]      # d2
    shl si, 1
    mov ax, word ptr [l_ci]
    mov bx, word ptr [l_cj]
    mov cx, ax
    add cx, word ptr [bomb_ox+si]    # ni
    mov dx, bx
    add dx, word ptr [bomb_oy+si]    # nj
    push cx
    push dx
    call glink
    pop dx
    pop cx
    mov ax, dx
    imul ax, ax, MAXNX
    add ax, cx
    mov bx, ax
    mov byte ptr [seen+bx], 1
    inc word ptr [l_top]
    mov bx, word ptr [l_top]
    shl bx, 1
    mov word ptr [lstack+bx], ax
    jmp .Lly_dfs

.Lly_braid:                          # extra passages -> loops
    mov ax, word ptr [l_hx]
    imul ax, word ptr [l_ny]
    # Keep fewer cross-passages so corridors stay narrow and deliberate.
    mov cx, 5
    xor dx, dx
    div cx
    mov bx, word ptr [l_depth]
    shr bx, 1
    add ax, bx
    mov word ptr [l_n], ax
.Lly_b1:
    cmp word ptr [l_n], 0
    jle .Lly_chamb
    dec word ptr [l_n]
    mov cx, word ptr [l_hx]
    call rnd
    mov word ptr [l_bi], ax
    mov cx, word ptr [l_ny]
    call rnd
    mov word ptr [l_bj], ax
    mov cx, 2
    call rnd
    test ax, ax
    jz .Lly_b2
    mov ax, word ptr [l_bi]
    inc ax
    cmp ax, word ptr [l_hx]
    jge .Lly_b2
    mov cx, ax                       # link(bi,bj,bi+1,bj)
    mov ax, word ptr [l_bi]
    mov bx, word ptr [l_bj]
    mov dx, bx
    call glink
    jmp .Lly_b1
.Lly_b2:
    mov bx, word ptr [l_bj]
    inc bx
    cmp bx, word ptr [l_ny]
    jge .Lly_b1
    mov dx, bx                       # link(bi,bj,bi,bj+1)
    mov ax, word ptr [l_bi]
    mov cx, ax
    mov bx, word ptr [l_bj]
    call glink
    jmp .Lly_b1

.Lly_chamb:                          # chambers with pillars
    mov ax, word ptr [l_hx]
    imul ax, word ptr [l_ny]
    xor dx, dx
    # Rare small chambers; prioritize tight, winding corridors over open rooms.
    mov cx, 10
    div cx
    mov word ptr [l_n], ax
.Lly_c1:
    cmp word ptr [l_n], 0
    jle .Lly_mirror
    dec word ptr [l_n]
    mov cx, 4
    call rnd
    add ax, 3
    mov word ptr [l_cw], ax
    mov cx, 3
    call rnd
    add ax, 3
    mov word ptr [l_ch], ax
    mov cx, word ptr [l_hx]          # x0 = 1 + 3*rnd(hx) - rnd(cw)
    call rnd
    imul ax, ax, 3
    inc ax
    mov word ptr [l_x0], ax
    mov cx, word ptr [l_cw]
    call rnd
    sub word ptr [l_x0], ax
    mov cx, word ptr [l_ny]          # y0 = 1 + 3*rnd(ny) - rnd(ch)
    call rnd
    imul ax, ax, 3
    inc ax
    mov word ptr [l_y0], ax
    mov cx, word ptr [l_ch]
    call rnd
    sub word ptr [l_y0], ax
    cmp word ptr [l_x0], 1
    jge .Lly_c2
    mov word ptr [l_x0], 1
.Lly_c2:
    cmp word ptr [l_y0], 2
    jge .Lly_c3
    mov word ptr [l_y0], 2
.Lly_c3:
    mov ax, word ptr [l_x0]
    add ax, word ptr [l_cw]
    dec ax
    mov word ptr [l_x1], ax
    mov ax, word ptr [l_y0]
    add ax, word ptr [l_ch]
    dec ax
    mov word ptr [l_y1], ax
    mov ax, word ptr [l_xmax]
    cmp word ptr [l_x1], ax
    jle .Lly_c4
    mov word ptr [l_x1], ax
.Lly_c4:
    mov ax, word ptr [l_h]
    sub ax, 3
    cmp word ptr [l_y1], ax
    jle .Lly_c5
    mov word ptr [l_y1], ax
.Lly_c5:
    mov ax, word ptr [l_x1]
    sub ax, word ptr [l_x0]
    cmp ax, 2
    jl .Lly_c1
    mov ax, word ptr [l_y1]
    sub ax, word ptr [l_y0]
    cmp ax, 2
    jl .Lly_c1
    mov ax, word ptr [l_x0]
    mov bx, word ptr [l_y0]
    mov cx, word ptr [l_x1]
    mov dx, word ptr [l_y1]
    mov di, T_FLOOR
    call gfill
    mov ax, word ptr [l_x1]          # np = (w*h)/7
    sub ax, word ptr [l_x0]
    inc ax
    mov bx, word ptr [l_y1]
    sub bx, word ptr [l_y0]
    inc bx
    imul ax, bx
    xor dx, dx
    mov cx, 7
    div cx
    mov word ptr [l_np], ax
    mov word ptr [l_k], 0
.Lly_p1:
    mov ax, word ptr [l_k]
    cmp ax, word ptr [l_np]
    jge .Lly_c1
    inc word ptr [l_k]
    mov cx, word ptr [l_x1]          # px = x0+1+rnd(x1-x0-1)
    sub cx, word ptr [l_x0]
    dec cx
    call rnd
    add ax, word ptr [l_x0]
    inc ax
    mov word ptr [l_px], ax
    mov cx, word ptr [l_y1]          # py = y0+1+rnd(y1-y0-1)
    sub cx, word ptr [l_y0]
    dec cx
    call rnd
    add ax, word ptr [l_y0]
    inc ax
    mov word ptr [l_py], ax
    mov ax, word ptr [l_xmax]
    dec ax
    cmp word ptr [l_px], ax
    jg .Lly_p1
    mov ax, word ptr [l_px]
    mov bx, word ptr [l_py]
    call gfree8
    test al, al
    jz .Lly_p1
    mov bp, word ptr [gp_r]
    mov si, word ptr [l_py]
    imul si, word ptr [l_w]
    add si, word ptr [l_px]
    mov byte ptr [bp+si+R_C], T_WALL
    jmp .Lly_p1

.Lly_mirror:
    cmp word ptr [l_mirror], 0
    je .Lly_done
    mov bp, word ptr [gp_r]
    xor cx, cx                       # y
.Lly_m1:
    cmp cx, word ptr [l_h]
    jge .Lly_m4
    mov si, cx
    imul si, word ptr [l_w]          # row start
    xor dx, dx                       # x
.Lly_m2:
    mov ax, word ptr [l_w]
    shr ax, 1
    cmp dx, ax
    jge .Lly_m3
    mov di, si
    add di, dx
    mov al, byte ptr [bp+di+R_C]
    mov di, word ptr [l_w]
    dec di
    sub di, dx
    add di, si
    mov byte ptr [bp+di+R_C], al
    inc dx
    jmp .Lly_m2
.Lly_m3:
    inc cx
    jmp .Lly_m1
.Lly_m4:
    test word ptr [l_nx], 1
    jnz .Lly_done
    mov ax, word ptr [l_ny]          # even width: join the halves in the middle
    shr ax, 2
    add ax, 2
    mov word ptr [l_n], ax
    mov word ptr [l_k], 0
.Lly_j1:
    mov ax, word ptr [l_k]
    cmp ax, word ptr [l_n]
    jge .Lly_done
    inc word ptr [l_k]
    mov cx, word ptr [l_ny]
    call rnd
    imul ax, ax, 3
    inc ax                           # y1
    mov bx, ax
    mov dx, ax
    mov ax, word ptr [l_w]
    shr ax, 1
    dec ax                           # cx = W/2-1
    mov cx, ax
    inc cx
    mov di, T_FLOOR
    call gfill
    jmp .Lly_j1
.Lly_done:
    ret

# ---------------------------------------------------------------- solver
# slide_len
slide_len:
    mov bp, word ptr [gp_r]
    mov ax, si
    xor dx, dx
    div word ptr [bp+R_W]
    mov word ptr [sl_x], dx
    mov word ptr [sl_y], ax
    mov word ptr [sl_n], 0
    mov bx, di
    shl bx, 1
    mov word ptr [sl_d], bx
.Lsl_loop:
    mov bx, word ptr [sl_d]
    mov ax, word ptr [sl_x]
    add ax, word ptr [bomb_ox+bx]    # nx
    mov cx, word ptr [sl_y]
    add cx, word ptr [bomb_oy+bx]    # ny
    cmp ax, word ptr [bp+R_EX]
    jne .Lsl1
    cmp cx, word ptr [bp+R_EY]
    jne .Lsl1
    mov ax, EDGE_EXIT
    ret
.Lsl1:
    test ax, ax
    js .Lsl_end
    test cx, cx
    js .Lsl_end
    cmp ax, word ptr [bp+R_W]
    jge .Lsl_end
    cmp cx, word ptr [bp+R_H]
    jge .Lsl_end
    mov si, cx
    imul si, word ptr [bp+R_W]
    add si, ax
    mov dl, byte ptr [bp+si+R_C]
    cmp dl, T_TRAP
    jne .Lsl2
    xor ax, ax                       # the dash would die: not a usable move
    ret
.Lsl2:
    cmp dl, T_FLOOR
    je .Lsl3
    cmp dl, T_COIN
    je .Lsl3
    cmp dl, T_SHIELD
    jne .Lsl_end
.Lsl3:
    mov word ptr [sl_x], ax
    mov word ptr [sl_y], cx
    inc word ptr [sl_n]
    jmp .Lsl_loop
.Lsl_end:
    mov ax, word ptr [sl_n]
    cmp ax, 250
    jle .Lsl_ret
    mov ax, 250
.Lsl_ret:
    ret

# good_test: SI = cell -> ZF clear if marked "can still reach the exit"
good_test:
    mov bx, si
    shr bx, 3
    mov cx, si
    and cl, 7
    mov al, 1
    shl al, cl
    test byte ptr [good+bx], al
    ret
good_set:
    mov bx, si
    shr bx, 3
    mov cx, si
    and cl, 7
    mov al, 1
    shl al, cl
    or byte ptr [good+bx], al
    ret

# edge_target:
edge_target:
    mov bp, word ptr [gp_r]
    mov bx, di
    shl bx, 1
    mov ax, word ptr [bomb_oy+bx]
    imul ax, word ptr [bp+R_W]
    add ax, word ptr [bomb_ox+bx]    # DY*w + DX
    mov bx, si
    shl bx, 2
    add bx, di
    movzx cx, byte ptr [edge+bx]
    imul ax, cx
    add ax, si
    ret

# room_solve
room_solve:
    mov bp, word ptr [gp_r]
    mov ax, word ptr [bp+R_W]
    imul ax, word ptr [bp+R_H]
    mov word ptr [sv_t], ax          # ncell
    push es
    push ds
    pop es
    mov di, offset gdist
    mov cx, ax
    mov al, 255
    cld
    rep stosb
    mov di, offset good
    mov cx, word ptr [sv_t]
    shr cx, 3
    inc cx
    xor al, al
    rep stosb
    pop es
    mov si, word ptr [bp+R_SY]
    imul si, word ptr [bp+R_W]
    add si, word ptr [bp+R_SX]
    mov byte ptr [gdist+si], 0
    mov word ptr [queue], si
    mov word ptr [sv_head], 0
    mov word ptr [sv_tail], 1
    mov word ptr [sv_exit], -1
.Lrs_bfs:
    mov ax, word ptr [sv_head]
    cmp ax, word ptr [sv_tail]
    jge .Lrs_fix
    mov bx, ax
    shl bx, 1
    mov si, word ptr [queue+bx]
    mov word ptr [sv_s], si
    inc word ptr [sv_head]
    xor di, di
.Lrs_d:
    mov word ptr [sv_d], di
    mov si, word ptr [sv_s]
    call slide_len
    mov word ptr [sv_len], ax
    mov si, word ptr [sv_s]
    shl si, 2
    add si, word ptr [sv_d]
    mov byte ptr [edge+si], al
    test ax, ax
    jz .Lrs_dnext
    cmp ax, EDGE_EXIT
    jne .Lrs_norm
    mov bx, word ptr [sv_s]
    movzx ax, byte ptr [gdist+bx]
    inc ax                           # dist[s] + 1
    cmp word ptr [sv_exit], 0
    jl .Lrs_setexit
    cmp ax, word ptr [sv_exit]
    jge .Lrs_dnext
.Lrs_setexit:
    mov word ptr [sv_exit], ax
    jmp .Lrs_dnext
.Lrs_norm:
    mov si, word ptr [sv_s]
    mov di, word ptr [sv_d]
    call edge_target
    mov bx, ax                       # t
    cmp byte ptr [gdist+bx], 255
    jne .Lrs_dnext
    mov si, word ptr [sv_s]
    movzx ax, byte ptr [gdist+si]
    inc ax
    cmp ax, 254
    jle .Lrs_dist
    mov ax, 254
.Lrs_dist:
    mov byte ptr [gdist+bx], al
    mov cx, word ptr [sv_tail]
    shl cx, 1
    mov si, cx
    mov word ptr [queue+si], bx
    inc word ptr [sv_tail]
.Lrs_dnext:
    mov di, word ptr [sv_d]
    inc di
    cmp di, 4
    jl .Lrs_d
    jmp .Lrs_bfs
.Lrs_fix:                            # propagate "can reach the exit" backwards until nothing changes
    mov word ptr [sv_changed], 0
    mov word ptr [sv_i], 0
.Lrs_f1:
    mov ax, word ptr [sv_i]
    cmp ax, word ptr [sv_tail]
    jge .Lrs_f_end
    mov bx, ax
    shl bx, 1
    mov si, word ptr [queue+bx]
    mov word ptr [sv_s], si
    inc word ptr [sv_i]
    call good_test
    jnz .Lrs_f1
    xor di, di
.Lrs_f2:
    mov word ptr [sv_d], di
    mov si, word ptr [sv_s]
    shl si, 2
    add si, di
    movzx ax, byte ptr [edge+si]
    test ax, ax
    jz .Lrs_f3
    cmp ax, EDGE_EXIT
    je .Lrs_fgood
    mov si, word ptr [sv_s]
    mov di, word ptr [sv_d]
    call edge_target
    mov si, ax
    call good_test
    jz .Lrs_f3
.Lrs_fgood:
    mov si, word ptr [sv_s]
    call good_set
    mov word ptr [sv_changed], 1
    jmp .Lrs_f1
.Lrs_f3:
    mov di, word ptr [sv_d]
    inc di
    cmp di, 4
    jl .Lrs_f2
    jmp .Lrs_f1
.Lrs_f_end:
    cmp word ptr [sv_changed], 0
    jne .Lrs_fix
    mov word ptr [sv_str], 0         # count the stranded rest positions
    xor bx, bx
.Lrs_s1:
    cmp bx, word ptr [sv_tail]
    jge .Lrs_ret
    push bx
    shl bx, 1
    mov si, word ptr [queue+bx]
    call good_test
    pop bx
    jnz .Lrs_s2
    inc word ptr [sv_str]
.Lrs_s2:
    inc bx
    jmp .Lrs_s1
.Lrs_ret:
    mov ax, word ptr [sv_exit]
    ret

# ---------------------------------------------------------------- traps and coins
# place_traps
place_traps:
    mov word ptr [pt_min], bx
    mov bx, ax
    inc bx                           # want = 1 + depth (max 14)
    cmp bx, 14
    jle .Lpt0
    mov bx, 14
.Lpt0:
    mov word ptr [pt_want], bx
    mov word ptr [pt_tries], 0
.Lpt_loop:
    cmp word ptr [pt_want], 0
    jle .Lpt_ret
    mov ax, word ptr [pt_want]       # tries++ < want*12+20
    imul ax, ax, 12
    add ax, 20
    mov bx, word ptr [pt_tries]
    inc word ptr [pt_tries]
    cmp bx, ax
    jge .Lpt_ret
    mov bp, word ptr [gp_r]
    mov cx, word ptr [bp+R_W]
    imul cx, word ptr [bp+R_H]
    call rnd
    mov word ptr [pt_c], ax
    xor dx, dx
    div word ptr [bp+R_W]
    mov word ptr [pt_x], dx
    mov word ptr [pt_y], ax
    mov di, word ptr [pt_c]
    cmp byte ptr [bp+di+R_C], T_FLOOR
    jne .Lpt_loop
    mov ax, word ptr [bp+R_SX]       # keep the start calm
    sub ax, 2
    cmp word ptr [pt_x], ax
    jl .Lpt_c2
    mov ax, word ptr [bp+R_SX]
    add ax, 2
    cmp word ptr [pt_x], ax
    jg .Lpt_c2
    mov ax, word ptr [bp+R_SY]
    sub ax, 3
    cmp word ptr [pt_y], ax
    jge .Lpt_loop
.Lpt_c2:
    mov ax, word ptr [pt_x]
    cmp ax, word ptr [bp+R_EX]
    jne .Lpt_try
    cmp word ptr [pt_y], 2
    jle .Lpt_loop
.Lpt_try:
    mov di, word ptr [pt_c]
    mov byte ptr [bp+di+R_C], T_TRAP
    call room_solve
    mov bp, word ptr [gp_r]
    cmp ax, word ptr [pt_min]
    jl .Lpt_undo
    cmp word ptr [sv_str], 0
    jne .Lpt_undo
    mov bx, word ptr [bp+R_SY]
    imul bx, word ptr [bp+R_W]
    add bx, word ptr [bp+R_SX]
    cmp byte ptr [gdist+bx], 0
    jne .Lpt_undo
    dec word ptr [pt_want]
    jmp .Lpt_loop
.Lpt_undo:
    mov di, word ptr [pt_c]
    mov byte ptr [bp+di+R_C], T_FLOOR
    jmp .Lpt_loop
.Lpt_ret:
    ret

# decorate: AX = depth.  Coin runs and shields on cells a dash really crosses.
decorate:
    mov word ptr [dc_tries], 0
    mov word ptr [dc_placed], 0
    mov bx, ax
    imul bx, bx, 2
    add bx, 10                       # want = 10 + depth*2 (max 44)
    cmp bx, 44
    jle .Ldc0
    mov bx, 44
.Ldc0:
    mov word ptr [dc_want], bx
    mov word ptr [dc_shields], 0
    mov word ptr [l_depth], ax       # keep depth around
    cmp ax, 2
    jl .Ldc1
    mov cx, 3
    call rnd
    test ax, ax
    jz .Ldc1
    mov word ptr [dc_shields], 1
.Ldc1:
    cmp word ptr [l_depth], 6
    jl .Ldc2
    mov cx, 2
    call rnd
    test ax, ax
    jz .Ldc2
    inc word ptr [dc_shields]
.Ldc2:
    call room_solve
.Ldc_loop:
    mov ax, word ptr [dc_placed]     # (placed < want || shields > 0) && tries++ < 400
    cmp ax, word ptr [dc_want]
    jl .Ldc_c1
    cmp word ptr [dc_shields], 0
    jle .Ldc_ret
.Ldc_c1:
    mov ax, word ptr [dc_tries]
    inc word ptr [dc_tries]
    cmp ax, 400
    jge .Ldc_ret
    mov bp, word ptr [gp_r]
    mov cx, word ptr [bp+R_W]
    imul cx, word ptr [bp+R_H]
    call rnd
    mov word ptr [dc_st], ax
    mov cx, 4
    call rnd
    mov word ptr [dc_d], ax
    mov bx, word ptr [dc_st]
    cmp byte ptr [gdist+bx], 255
    je .Ldc_loop
    shl bx, 2
    add bx, word ptr [dc_d]
    movzx ax, byte ptr [edge+bx]
    mov word ptr [dc_len], ax
    test ax, ax
    jz .Ldc_loop
    cmp ax, EDGE_EXIT
    je .Ldc_loop
    mov word ptr [dc_run], 1
    cmp word ptr [dc_shields], 0
    jg .Ldc_r1
    mov cx, 4
    call rnd
    add ax, 2
    mov word ptr [dc_run], ax
.Ldc_r1:
    mov ax, word ptr [dc_len]
    cmp word ptr [dc_run], ax
    jle .Ldc_r2
    mov word ptr [dc_run], ax
.Ldc_r2:
    mov cx, word ptr [dc_len]
    sub cx, word ptr [dc_run]
    inc cx
    call rnd
    mov word ptr [dc_off], ax
    mov bp, word ptr [gp_r]
    mov ax, word ptr [dc_st]         # x = st%w + DX*(off+1), y = st/w + DY*(off+1)
    xor dx, dx
    div word ptr [bp+R_W]
    mov word ptr [dc_y], ax
    mov word ptr [dc_x], dx
    mov bx, word ptr [dc_d]
    shl bx, 1
    mov cx, word ptr [dc_off]
    inc cx
    mov ax, word ptr [bomb_ox+bx]
    imul ax, cx
    add word ptr [dc_x], ax
    mov ax, word ptr [bomb_oy+bx]
    imul ax, cx
    add word ptr [dc_y], ax
    mov word ptr [dc_k], 0
.Ldc_k:
    mov ax, word ptr [dc_k]
    cmp ax, word ptr [dc_run]
    jge .Ldc_loop
    mov si, word ptr [dc_y]
    imul si, word ptr [bp+R_W]
    add si, word ptr [dc_x]
    cmp byte ptr [bp+si+R_C], T_FLOOR
    jne .Ldc_knext
    mov ax, word ptr [dc_x]
    cmp ax, word ptr [bp+R_SX]
    jne .Ldc_k2
    mov ax, word ptr [dc_y]
    cmp ax, word ptr [bp+R_SY]
    je .Ldc_knext
.Ldc_k2:
    cmp word ptr [dc_shields], 0
    jle .Ldc_coin
    mov byte ptr [bp+si+R_C], T_SHIELD
    dec word ptr [dc_shields]
    jmp .Ldc_loop
.Ldc_coin:
    mov byte ptr [bp+si+R_C], T_COIN
    inc word ptr [dc_placed]
.Ldc_knext:
    inc word ptr [dc_k]
    mov bx, word ptr [dc_d]
    shl bx, 1
    mov ax, word ptr [bomb_ox+bx]
    add word ptr [dc_x], ax
    mov ax, word ptr [bomb_oy+bx]
    add word ptr [dc_y], ax
    jmp .Ldc_k
.Ldc_ret:
    ret

# place_key_bomb: add the exit lock and the optional puzzle items.
# The exit corridor gets a door, a gate and a toggle wall, so the key, plate
# and lever all lead to the exit.  The key, plate, lever and bomb stay in the
# main room so they can be collected before the climb.
place_key_bomb:
    mov bp, word ptr [gp_r]
    mov ax, word ptr [bp+R_SY]
    imul ax, word ptr [bp+R_W]
    add ax, word ptr [bp+R_SX]
    mov word ptr [dc_st], ax
    mov ax, word ptr [bp+R_W]
    imul ax, word ptr [bp+R_H]
    mov word ptr [dc_run], ax
    mov word ptr [kb_key], 0xFFFF
    mov word ptr [kb_plate], 0xFFFF
    mov word ptr [kb_lever], 0xFFFF
    mov word ptr [kb_bomb], 0xFFFF
    mov word ptr [kb_crate], 0xFFFF
    mov word ptr [bp+R_NFLIP], 0
    cmp word ptr [bp+R_EY], 0
    jne .Lpkb_abort
    cmp word ptr [bp+R_H], 5
    jl .Lpkb_abort

    # Clear the five-cell exit corridor so traps or pickups cannot sit in it.
    mov cx, 1
.Lpkb_clear:
    cmp cx, 5
    jge .Lpkb_clear_done
    mov ax, cx
    imul ax, word ptr [bp+R_W]
    add ax, word ptr [bp+R_EX]
    mov di, ax
    mov byte ptr [bp+di+R_C], T_FLOOR
    inc cx
    jmp .Lpkb_clear
.Lpkb_clear_done:

    # Seal the sides of the three lock cells.  The only entrance is the bottom
    # cell (ex,4), so the player can never stand between two locks or enter
    # beside the gate.
    mov cx, 1
.Lpkb_seal:
    cmp cx, 4
    jge .Lpkb_seal_done
    mov ax, cx
    imul ax, word ptr [bp+R_W]
    add ax, word ptr [bp+R_EX]
    mov di, ax
    dec di
    mov bx, cx
    dec bx
    shl bx, 1
    mov al, byte ptr [bp+di+R_C]
    mov byte ptr [kb_save+bx], al
    mov byte ptr [bp+di+R_C], T_WALL
    mov ax, cx
    imul ax, word ptr [bp+R_W]
    add ax, word ptr [bp+R_EX]
    mov di, ax
    inc di
    mov al, byte ptr [bp+di+R_C]
    mov byte ptr [kb_save+bx+1], al
    mov byte ptr [bp+di+R_C], T_WALL
    inc cx
    jmp .Lpkb_seal
.Lpkb_seal_done:
    call room_solve
    test ax, ax
    js .Lpkb_seal_fail
    cmp word ptr [sv_str], 0
    jne .Lpkb_seal_fail
    jmp .Lpkb_seal_ok
.Lpkb_seal_fail:
    mov cx, 1
.Lpkb_unseal:
    cmp cx, 4
    jge .Lpkb_unseal_done
    mov ax, cx
    imul ax, word ptr [bp+R_W]
    add ax, word ptr [bp+R_EX]
    mov di, ax
    dec di
    mov bx, cx
    dec bx
    shl bx, 1
    mov al, byte ptr [kb_save+bx]
    mov byte ptr [bp+di+R_C], al
    mov ax, cx
    imul ax, word ptr [bp+R_W]
    add ax, word ptr [bp+R_EX]
    mov di, ax
    inc di
    mov al, byte ptr [kb_save+bx+1]
    mov byte ptr [bp+di+R_C], al
    inc cx
    jmp .Lpkb_unseal
.Lpkb_unseal_done:
    call room_solve
    xor ax, ax
    ret
.Lpkb_seal_ok:

    mov al, T_KEY
    call pkb_find_item
    mov word ptr [kb_key], ax
    cmp ax, 0xFFFF
    je .Lpkb_abort
    mov al, T_PLATE
    call pkb_find_item
    mov word ptr [kb_plate], ax
    cmp ax, 0xFFFF
    je .Lpkb_abort
    mov al, T_LEVER
    call pkb_find_item
    mov word ptr [kb_lever], ax
    cmp ax, 0xFFFF
    je .Lpkb_abort
    mov al, T_BOMB
    call pkb_find_item
    mov word ptr [kb_bomb], ax

    # The crate alcove experiment is disabled: it made the self-test hang.
    # The exit lock and side-wall validation below are the tested path.
    # call pkb_carve_crate

    # The key/door, plate/gate and lever/toggle form three layers directly
    # under the exit.  The player enters the shaft only from (ex,4).
    mov ax, word ptr [bp+R_W]
    add ax, word ptr [bp+R_EX]
    mov di, ax
    mov byte ptr [bp+di+R_C], T_DOOR
    mov ax, word ptr [bp+R_W]
    shl ax, 1
    add ax, word ptr [bp+R_EX]
    mov di, ax
    mov byte ptr [bp+di+R_C], T_GATE
    mov ax, word ptr [bp+R_W]
    imul ax, ax, 3
    add ax, word ptr [bp+R_EX]
    mov di, ax
    mov byte ptr [bp+di+R_C], T_TOGGLE
    mov word ptr [bp+R_NFLIP], 1
    mov word ptr [bp+R_FLIP], di
    mov ax, 1
    ret
.Lpkb_abort:
    xor ax, ax
    ret

# pkb_find_item: AL = tile, returns AX = cell index or 0xFFFF.
# Only reachable floor cells outside the exit column are used.
pkb_find_item:
    mov byte ptr [kb_tile], al
    xor si, si
.Lpfi_loop:
    cmp si, word ptr [dc_run]
    jge .Lpfi_none
    cmp si, word ptr [dc_st]
    je .Lpfi_next
    cmp byte ptr [bp+si+R_C], T_FLOOR
    jne .Lpfi_next
    cmp byte ptr [gdist+si], 255
    je .Lpfi_next
    mov ax, si
    xor dx, dx
    div word ptr [bp+R_W]
    cmp dx, word ptr [bp+R_EX]
    je .Lpfi_next
    mov al, byte ptr [kb_tile]
    mov byte ptr [bp+si+R_C], al
    mov ax, si
    ret
.Lpfi_next:
    inc si
    jmp .Lpfi_loop
.Lpfi_none:
    mov ax, 0xFFFF
    ret

# pkb_safe_crate: put a crate in a one-cell side alcove.  The crate can only be
# pushed into the dead end, never back across the main path, so it cannot seal
# the room.
pkb_safe_crate:
    xor si, si
.Lpsc_loop:
    cmp si, word ptr [dc_run]
    jge .Lpsc_ret
    cmp si, word ptr [dc_st]
    je .Lpsc_next
    cmp byte ptr [bp+si+R_C], T_FLOOR
    jne .Lpsc_next
    cmp byte ptr [gdist+si], 255
    je .Lpsc_next
    mov ax, si
    xor dx, dx
    div word ptr [bp+R_W]
    mov word ptr [dc_y], ax
    mov word ptr [dc_x], dx
    cmp dx, word ptr [bp+R_EX]
    je .Lpsc_next
    call pkb_degree
    cmp ax, 2
    jne .Lpsc_next

    mov di, 0xFFFF
    mov bx, si
    mov ax, word ptr [dc_x]
    test ax, ax
    jz .Lpsc_tip_left_done
    dec bx
    call pkb_is_tip
    test ax, ax
    jz .Lpsc_tip_left_done
    mov di, bx
.Lpsc_tip_left_done:
    mov bx, si
    mov ax, word ptr [dc_x]
    inc ax
    cmp ax, word ptr [bp+R_W]
    jge .Lpsc_tip_right_done
    inc bx
    call pkb_is_tip
    test ax, ax
    jz .Lpsc_tip_right_done
    mov di, bx
.Lpsc_tip_right_done:
    mov bx, si
    mov ax, word ptr [dc_y]
    test ax, ax
    jz .Lpsc_tip_up_done
    sub bx, word ptr [bp+R_W]
    call pkb_is_tip
    test ax, ax
    jz .Lpsc_tip_up_done
    mov di, bx
.Lpsc_tip_up_done:
    mov bx, si
    mov ax, word ptr [dc_y]
    inc ax
    cmp ax, word ptr [bp+R_H]
    jge .Lpsc_tip_down_done
    add bx, word ptr [bp+R_W]
    call pkb_is_tip
    test ax, ax
    jz .Lpsc_tip_down_done
    mov di, bx
.Lpsc_tip_down_done:
    cmp di, 0xFFFF
    je .Lpsc_next

    # The other floor neighbour is the path side; it must be a through cell.
    mov bx, si
    mov ax, word ptr [dc_x]
    test ax, ax
    jz .Lpsc_path_left_done
    dec bx
    cmp bx, di
    je .Lpsc_path_left_done
    push si
    mov si, bx
    cmp byte ptr [bp+si+R_C], T_FLOOR
    pop si
    jne .Lpsc_path_left_done
    push si
    mov si, bx
    call pkb_degree
    pop si
    cmp ax, 2
    jge .Lpsc_place
.Lpsc_path_left_done:
    mov bx, si
    mov ax, word ptr [dc_x]
    inc ax
    cmp ax, word ptr [bp+R_W]
    jge .Lpsc_path_right_done
    inc bx
    cmp bx, di
    je .Lpsc_path_right_done
    push si
    mov si, bx
    cmp byte ptr [bp+si+R_C], T_FLOOR
    pop si
    jne .Lpsc_path_right_done
    push si
    mov si, bx
    call pkb_degree
    pop si
    cmp ax, 2
    jge .Lpsc_place
.Lpsc_path_right_done:
    mov bx, si
    mov ax, word ptr [dc_y]
    test ax, ax
    jz .Lpsc_path_up_done
    sub bx, word ptr [bp+R_W]
    cmp bx, di
    je .Lpsc_path_up_done
    push si
    mov si, bx
    cmp byte ptr [bp+si+R_C], T_FLOOR
    pop si
    jne .Lpsc_path_up_done
    push si
    mov si, bx
    call pkb_degree
    pop si
    cmp ax, 2
    jge .Lpsc_place
.Lpsc_path_up_done:
    mov bx, si
    mov ax, word ptr [dc_y]
    inc ax
    cmp ax, word ptr [bp+R_H]
    jge .Lpsc_path_down_done
    add bx, word ptr [bp+R_W]
    cmp bx, di
    je .Lpsc_path_down_done
    push si
    mov si, bx
    cmp byte ptr [bp+si+R_C], T_FLOOR
    pop si
    jne .Lpsc_path_down_done
    push si
    mov si, bx
    call pkb_degree
    pop si
    cmp ax, 2
    jge .Lpsc_place
.Lpsc_path_down_done:
    jmp .Lpsc_next
.Lpsc_place:
    mov byte ptr [bp+si+R_C], T_CRATE
    mov word ptr [kb_crate], si
    ret
.Lpsc_next:
    inc si
    jmp .Lpsc_loop
.Lpsc_ret:
    ret

# pkb_is_tip: BX = neighbour, returns AX = 1 when it is a reachable one-cell
# dead end that is not the start and not in the exit column.
pkb_is_tip:
    push si
    mov si, bx
    cmp byte ptr [bp+si+R_C], T_FLOOR
    jne .Lpit_no
    cmp si, word ptr [dc_st]
    je .Lpit_no
    mov ax, si
    xor dx, dx
    div word ptr [bp+R_W]
    cmp dx, word ptr [bp+R_EX]
    je .Lpit_no
    call pkb_degree
    cmp ax, 1
    jne .Lpit_no
    mov ax, 1
    pop si
    ret
.Lpit_no:
    xor ax, ax
    pop si
    ret

# pkb_degree: SI = cell, returns AX = number of floor neighbours.
pkb_degree:
    push bx
    push cx
    push dx
    push di
    mov word ptr [kb_tmp], si
    mov ax, si
    xor dx, dx
    div word ptr [bp+R_W]
    mov cx, ax
    mov bx, dx
    xor di, di
    test bx, bx
    jz .Lpd_left_done
    mov si, word ptr [kb_tmp]
    dec si
    cmp byte ptr [bp+si+R_C], T_FLOOR
    jne .Lpd_left_done
    inc di
.Lpd_left_done:
    mov ax, bx
    inc ax
    cmp ax, word ptr [bp+R_W]
    jge .Lpd_right_done
    mov si, word ptr [kb_tmp]
    inc si
    cmp byte ptr [bp+si+R_C], T_FLOOR
    jne .Lpd_right_done
    inc di
.Lpd_right_done:
    test cx, cx
    jz .Lpd_up_done
    mov si, word ptr [kb_tmp]
    sub si, word ptr [bp+R_W]
    cmp byte ptr [bp+si+R_C], T_FLOOR
    jne .Lpd_up_done
    inc di
.Lpd_up_done:
    mov ax, cx
    inc ax
    cmp ax, word ptr [bp+R_H]
    jge .Lpd_down_done
    mov si, word ptr [kb_tmp]
    add si, word ptr [bp+R_W]
    cmp byte ptr [bp+si+R_C], T_FLOOR
    jne .Lpd_down_done
    inc di
.Lpd_down_done:
    mov si, word ptr [kb_tmp]
    mov ax, di
    pop di
    pop dx
    pop cx
    pop bx
    ret

# pkb_safe_crate2: a crate in a straight alcove whose two floor neighbours are
# opposite.  One neighbour is the path, the other is a dead end, so the crate
# can only be pushed into the dead end and can never block the path.
pkb_safe_crate2:
    xor si, si
.Lpsc2_loop:
    cmp si, word ptr [dc_run]
    jge .Lpsc2_ret
    cmp si, word ptr [dc_st]
    je .Lpsc2_next
    cmp byte ptr [bp+si+R_C], T_FLOOR
    jne .Lpsc2_next
    cmp byte ptr [gdist+si], 255
    je .Lpsc2_next
    mov ax, si
    xor dx, dx
    div word ptr [bp+R_W]
    mov word ptr [dc_y], ax
    mov word ptr [dc_x], dx
    cmp dx, word ptr [bp+R_EX]
    je .Lpsc2_next
    call pkb_degree
    cmp ax, 2
    jne .Lpsc2_next

    mov ax, word ptr [dc_x]
    test ax, ax
    jz .Lpsc2_vertical
    inc ax
    cmp ax, word ptr [bp+R_W]
    jge .Lpsc2_vertical
    mov bx, si
    dec bx
    call pkb_is_tip
    test ax, ax
    jz .Lpsc2_h_right_tip
    mov bx, si
    inc bx
    call pkb_is_path
    test ax, ax
    jnz .Lpsc2_place
    jmp .Lpsc2_vertical
.Lpsc2_h_right_tip:
    mov bx, si
    inc bx
    call pkb_is_tip
    test ax, ax
    jz .Lpsc2_vertical
    mov bx, si
    dec bx
    call pkb_is_path
    test ax, ax
    jnz .Lpsc2_place
.Lpsc2_vertical:
    mov ax, word ptr [dc_y]
    test ax, ax
    jz .Lpsc2_next
    inc ax
    cmp ax, word ptr [bp+R_H]
    jge .Lpsc2_next
    mov bx, si
    sub bx, word ptr [bp+R_W]
    call pkb_is_tip
    test ax, ax
    jz .Lpsc2_v_down_tip
    mov bx, si
    add bx, word ptr [bp+R_W]
    call pkb_is_path
    test ax, ax
    jnz .Lpsc2_place
    jmp .Lpsc2_next
.Lpsc2_v_down_tip:
    mov bx, si
    add bx, word ptr [bp+R_W]
    call pkb_is_tip
    test ax, ax
    jz .Lpsc2_next
    mov bx, si
    sub bx, word ptr [bp+R_W]
    call pkb_is_path
    test ax, ax
    jnz .Lpsc2_place
.Lpsc2_next:
    inc si
    jmp .Lpsc2_loop
.Lpsc2_place:
    mov byte ptr [bp+si+R_C], T_CRATE
    ret
.Lpsc2_ret:
    ret

# pkb_is_path: BX = neighbour, AX = 1 when it is a floor through cell.
pkb_is_path:
    push si
    mov si, bx
    cmp byte ptr [bp+si+R_C], T_FLOOR
    jne .Lpip_no
    call pkb_degree
    cmp ax, 2
    jl .Lpip_no
    mov ax, 1
    pop si
    ret
.Lpip_no:
    xor ax, ax
    pop si
    ret

# pkb_carve_crate: carve a two-cell wall pocket beside a reachable floor cell
# and put a crate in the first cell.  The sides and the far end are walls, so
# the crate can only be pushed into the dead end and never across the path.
pkb_carve_crate:
    xor si, si
.Lpcc_loop:
    cmp si, word ptr [dc_run]
    jge .Lpcc_ret
    cmp si, word ptr [dc_st]
    je .Lpcc_next
    cmp byte ptr [bp+si+R_C], T_FLOOR
    jne .Lpcc_next
    cmp byte ptr [gdist+si], 255
    je .Lpcc_next
    mov word ptr [kb_p], si
    mov ax, si
    xor dx, dx
    div word ptr [bp+R_W]
    mov word ptr [dc_y], ax
    mov word ptr [dc_x], dx

    cmp word ptr [dc_x], 3
    jl .Lpcc_right
    cmp word ptr [dc_y], 5
    jl .Lpcc_right
    mov ax, word ptr [bp+R_H]
    dec ax
    cmp word ptr [dc_y], ax
    jg .Lpcc_right
    mov ax, word ptr [dc_x]
    dec ax
    cmp ax, word ptr [bp+R_EX]
    je .Lpcc_right
    dec ax
    cmp ax, word ptr [bp+R_EX]
    je .Lpcc_right
    mov si, word ptr [kb_p]
    dec si
    mov word ptr [kb_c], si
    cmp byte ptr [bp+si+R_C], T_WALL
    jne .Lpcc_right
    dec si
    mov word ptr [kb_t], si
    cmp byte ptr [bp+si+R_C], T_WALL
    jne .Lpcc_right
    mov si, word ptr [kb_c]
    sub si, word ptr [bp+R_W]
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_right
    mov si, word ptr [kb_c]
    add si, word ptr [bp+R_W]
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_right
    mov si, word ptr [kb_t]
    sub si, word ptr [bp+R_W]
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_right
    mov si, word ptr [kb_t]
    add si, word ptr [bp+R_W]
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_right
    mov si, word ptr [kb_t]
    dec si
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_right
    jmp .Lpcc_place

.Lpcc_right:
    mov ax, word ptr [bp+R_W]
    sub ax, 4
    cmp word ptr [dc_x], ax
    jg .Lpcc_vertical
    cmp word ptr [dc_y], 5
    jl .Lpcc_vertical
    mov ax, word ptr [bp+R_H]
    dec ax
    cmp word ptr [dc_y], ax
    jg .Lpcc_vertical
    mov ax, word ptr [dc_x]
    inc ax
    cmp ax, word ptr [bp+R_EX]
    je .Lpcc_vertical
    inc ax
    cmp ax, word ptr [bp+R_EX]
    je .Lpcc_vertical
    mov si, word ptr [kb_p]
    inc si
    mov word ptr [kb_c], si
    cmp byte ptr [bp+si+R_C], T_WALL
    jne .Lpcc_vertical
    inc si
    mov word ptr [kb_t], si
    cmp byte ptr [bp+si+R_C], T_WALL
    jne .Lpcc_vertical
    mov si, word ptr [kb_c]
    sub si, word ptr [bp+R_W]
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_vertical
    mov si, word ptr [kb_c]
    add si, word ptr [bp+R_W]
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_vertical
    mov si, word ptr [kb_t]
    sub si, word ptr [bp+R_W]
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_vertical
    mov si, word ptr [kb_t]
    add si, word ptr [bp+R_W]
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_vertical
    mov si, word ptr [kb_t]
    inc si
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_vertical
    jmp .Lpcc_place

.Lpcc_vertical:
    cmp word ptr [dc_y], 7
    jl .Lpcc_down
    mov ax, word ptr [dc_x]
    cmp ax, word ptr [bp+R_EX]
    je .Lpcc_down
    mov si, word ptr [kb_p]
    sub si, word ptr [bp+R_W]
    mov word ptr [kb_c], si
    cmp byte ptr [bp+si+R_C], T_WALL
    jne .Lpcc_down
    sub si, word ptr [bp+R_W]
    mov word ptr [kb_t], si
    cmp byte ptr [bp+si+R_C], T_WALL
    jne .Lpcc_down
    mov si, word ptr [kb_c]
    dec si
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_down
    mov si, word ptr [kb_c]
    inc si
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_down
    mov si, word ptr [kb_t]
    dec si
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_down
    mov si, word ptr [kb_t]
    inc si
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_down
    mov si, word ptr [kb_t]
    sub si, word ptr [bp+R_W]
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_down
    jmp .Lpcc_place

.Lpcc_down:
    mov ax, word ptr [bp+R_H]
    sub ax, 4
    cmp word ptr [dc_y], ax
    jg .Lpcc_next
    cmp word ptr [dc_y], 4
    jl .Lpcc_next
    mov ax, word ptr [dc_x]
    cmp ax, word ptr [bp+R_EX]
    je .Lpcc_next
    mov si, word ptr [kb_p]
    add si, word ptr [bp+R_W]
    mov word ptr [kb_c], si
    cmp byte ptr [bp+si+R_C], T_WALL
    jne .Lpcc_next
    add si, word ptr [bp+R_W]
    mov word ptr [kb_t], si
    cmp byte ptr [bp+si+R_C], T_WALL
    jne .Lpcc_next
    mov si, word ptr [kb_c]
    dec si
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_next
    mov si, word ptr [kb_c]
    inc si
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_next
    mov si, word ptr [kb_t]
    dec si
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_next
    mov si, word ptr [kb_t]
    inc si
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_next
    mov si, word ptr [kb_t]
    add si, word ptr [bp+R_W]
    cmp byte ptr [bp+si+R_C], T_FLOOR
    je .Lpcc_next

.Lpcc_place:
    mov si, word ptr [kb_c]
    mov byte ptr [bp+si+R_C], T_FLOOR
    mov si, word ptr [kb_t]
    mov byte ptr [bp+si+R_C], T_FLOOR
    mov si, word ptr [kb_c]
    mov byte ptr [bp+si+R_C], T_CRATE
    ret
.Lpcc_next:
    mov si, word ptr [kb_p]
    inc si
    jmp .Lpcc_loop
.Lpcc_ret:
    ret

# gen_room
gen_room:
    mov word ptr [gp_r], di
    mov word ptr [gd_depth], ax
    mov bx, ax                       # nx = 7 + min(depth/2, 5)
    shr bx, 1
    cmp bx, 5
    jle .Lgen0
    mov bx, 5
.Lgen0:
    add bx, 5
    mov word ptr [g_nx], bx
    mov bx, ax                       # ny = 8 + min(depth, 8)
    cmp bx, 8
    jle .Lgen1
    mov bx, 8
.Lgen1:
    add bx, 7
    mov word ptr [g_ny], bx
    mov word ptr [g_attempt], 0
.Lgen_try:
    cmp word ptr [g_attempt], 200
    jge .Lgen_ret
    mov cx, 5
    call rnd
    xor bx, bx
    cmp ax, 3
    jge .Lgen2
    mov bx, 1
.Lgen2:
    mov word ptr [g_mirror], bx
    mov word ptr [l_mirror], bx
    mov ax, word ptr [g_nx]
    mov word ptr [l_nx], ax
    mov ax, word ptr [g_ny]
    mov word ptr [l_ny], ax
    mov ax, word ptr [gd_depth]
    mov word ptr [l_depth], ax
    call layout
    mov cx, word ptr [g_nx]          # is = rnd(nx)
    call rnd
    mov word ptr [g_is], ax
    mov bx, 0                        # oddmirror = mirror && (nx & 1)
    cmp word ptr [g_mirror], 0
    je .Lgen3
    test word ptr [g_nx], 1
    jz .Lgen3
    mov bx, 1
.Lgen3:
    mov dx, bx                       # DX = oddmirror (kept across rnd: rnd preserves DX? no, store)
    push bx
    test bx, bx
    jz .Lgen4
    mov ax, word ptr [g_nx]
    shr ax, 1
    cmp word ptr [g_is], ax
    jne .Lgen4
    mov ax, word ptr [g_is]          # is = (is+1) % nx
    inc ax
    xor dx, dx
    div word ptr [g_nx]
    mov word ptr [g_is], dx
.Lgen4:
    pop bx
    test bx, bx
    jz .Lgen5
    mov ax, word ptr [g_nx]          # ie = nx/2
    shr ax, 1
    mov word ptr [g_ie], ax
    jmp .Lgen6
.Lgen5:
    mov cx, word ptr [g_nx]
    call rnd
    mov word ptr [g_ie], ax
    mov ax, word ptr [g_ie]          # !mirror && ie == is -> ie = (is + nx/2) % nx
    cmp word ptr [g_mirror], 0
    jne .Lgen6
    cmp ax, word ptr [g_is]
    jne .Lgen6
    mov ax, word ptr [g_nx]
    shr ax, 1
    add ax, word ptr [g_is]
    xor dx, dx
    div word ptr [g_nx]
    mov word ptr [g_ie], dx
.Lgen6:
    mov bp, word ptr [gp_r]
    mov ax, word ptr [g_is]
    imul ax, ax, 3
    inc ax
    mov word ptr [bp+R_SX], ax
    mov ax, word ptr [bp+R_H]
    sub ax, 2
    mov word ptr [bp+R_SY], ax
    mov ax, word ptr [g_ie]
    imul ax, ax, 3
    inc ax
    mov word ptr [bp+R_EX], ax
    mov word ptr [bp+R_EY], 0
    xor bx, bx                       # fill(ex,0,ex,4,floor): the climb to the exit
    mov cx, ax
    mov dx, 4
    mov di, T_FLOOR
    call gfill
    mov bp, word ptr [gp_r]
    mov word ptr [bp+R_NFLIP], 0
    mov ax, word ptr [gd_depth]      # min_exit = 5 + depth/3 - attempt/6 (at least 2)
    xor dx, dx
    mov cx, 3
    div cx
    add ax, 5
    mov bx, ax
    mov ax, word ptr [g_attempt]
    xor dx, dx
    mov cx, 6
    div cx
    sub bx, ax
    cmp bx, 2
    jge .Lgen7
    mov bx, 2
.Lgen7:
    mov word ptr [g_minexit], bx
    call room_solve
    cmp ax, word ptr [g_minexit]
    jl .Lgen_next
    cmp word ptr [sv_str], 0
    jne .Lgen_next
    mov ax, word ptr [gd_depth]
    mov bx, word ptr [g_minexit]
    call place_traps
    mov ax, word ptr [gd_depth]
    call decorate
    call place_key_bomb
    test ax, ax
    jz .Lgen_next
    ret
.Lgen_next:
    inc word ptr [g_attempt]
    jmp .Lgen_try
.Lgen_ret:
    ret
