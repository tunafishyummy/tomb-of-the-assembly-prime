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

#puzzle items
place_key_bomb:
    mov bp, word ptr [gp_r]
    mov ax, word ptr [bp+R_SY]
    imul ax, word ptr [bp+R_W]
    add ax, word ptr [bp+R_SX]
    mov word ptr [dc_st], ax
    mov ax, word ptr [bp+R_EY]
    imul ax, word ptr [bp+R_W]
    add ax, word ptr [bp+R_EX]
    mov word ptr [dc_len], ax
    mov ax, word ptr [bp+R_W]
    imul ax, word ptr [bp+R_H]
    mov word ptr [dc_run], ax
    mov word ptr [dc_placed], 0
    xor si, si
.Lpkb_loop:
    cmp si, word ptr [dc_run]
    jge .Lpkb_done
    cmp si, word ptr [dc_st]
    je .Lpkb_next
    cmp si, word ptr [dc_len]
    je .Lpkb_next
    cmp byte ptr [bp+si+R_C], T_FLOOR
    jne .Lpkb_next
    # Ignore unreachable floor cells.
    cmp byte ptr [gdist+si], 255
    je .Lpkb_next
    mov ax, word ptr [dc_placed]
    cmp ax, 0
    je .Lpkb_key
    cmp ax, 1
    je .Lpkb_door
    cmp ax, 2
    je .Lpkb_bomb
    cmp ax, 3
    je .Lpkb_crate
    jmp .Lpkb_done
.Lpkb_key:
    mov byte ptr [bp+si+R_C], T_KEY
    movzx ax, byte ptr [gdist+si]
    mov word ptr [dc_off], ax
    inc word ptr [dc_placed]
    jmp .Lpkb_next
.Lpkb_door:
    movzx ax, byte ptr [gdist+si]
    cmp ax, word ptr [dc_off]
    jle .Lpkb_next
    mov byte ptr [bp+si+R_C], T_DOOR
    inc word ptr [dc_placed]
    jmp .Lpkb_next
.Lpkb_bomb:
    mov byte ptr [bp+si+R_C], T_BOMB
    inc word ptr [dc_placed]
    jmp .Lpkb_next
.Lpkb_crate:
    # Only put a crate in a roomy spot so it is less likely to seal a corridor.
    mov ax, si
    xor dx, dx
    div word ptr [bp+R_W]
    mov word ptr [dc_y], ax
    mov word ptr [dc_x], dx
    mov ax, dx
    dec ax
    cmp ax, 0
    jl .Lpkb_next
    mov bx, word ptr [dc_y]
    imul bx, word ptr [bp+R_W]
    add bx, ax
    mov di, bx
    cmp byte ptr [bp+di+R_C], T_FLOOR
    jne .Lpkb_next
    mov ax, word ptr [dc_x]
    inc ax
    cmp ax, word ptr [bp+R_W]
    jge .Lpkb_next
    mov bx, word ptr [dc_y]
    imul bx, word ptr [bp+R_W]
    add bx, ax
    mov di, bx
    cmp byte ptr [bp+di+R_C], T_FLOOR
    jne .Lpkb_next
    mov ax, word ptr [dc_y]
    dec ax
    cmp ax, 0
    jl .Lpkb_next
    imul ax, word ptr [bp+R_W]
    add ax, word ptr [dc_x]
    mov di, ax
    cmp byte ptr [bp+di+R_C], T_FLOOR
    jne .Lpkb_next
    mov ax, word ptr [dc_y]
    inc ax
    cmp ax, word ptr [bp+R_H]
    jge .Lpkb_next
    imul ax, word ptr [bp+R_W]
    add ax, word ptr [dc_x]
    mov di, ax
    cmp byte ptr [bp+di+R_C], T_FLOOR
    jne .Lpkb_next
    mov byte ptr [bp+si+R_C], T_CRATE
    jmp .Lpkb_done
.Lpkb_next:
    inc si
    jmp .Lpkb_loop
.Lpkb_done:
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
    ret
.Lgen_next:
    inc word ptr [g_attempt]
    jmp .Lgen_try
.Lgen_ret:
    ret
