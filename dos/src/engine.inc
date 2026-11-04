# ============================================================================
#  Game rules (dash, crates, doors, plates, levers, bombs, shield, traps).
#  Works on the current room 'room' and the g_* game variables.
# ============================================================================
    .text
# copy_room: SI=source room, DI=destination room (both in the data segment)
copy_room:
    push es
    push ds
    pop es
    mov cx, ROOMSZ/2
    cld
    rep movsw
    pop es
    ret

# room_load_level: AX = level index (0..4), DI = destination room
room_load_level:
    mov word ptr [rl_dst], di
    mov word ptr [rl_idx], ax
    mov word ptr [di+R_W], 40
    mov word ptr [di+R_H], 28
    imul si, ax, 1120
    add si, offset level_data
    add di, R_C
    push es
    push ds
    pop es
    mov cx, 560
    cld
    rep movsw
    pop es
    mov di, word ptr [rl_dst]
    mov bx, word ptr [rl_idx]
    shl bx, 1
    mov ax, word ptr [level_start+bx]       # start cell = y*40+x
    xor dx, dx
    mov cx, 40
    div cx
    mov word ptr [di+R_SX], dx
    mov word ptr [di+R_SY], ax
    mov ax, word ptr [level_exit+bx]
    xor dx, dx
    div cx
    mov word ptr [di+R_EX], dx
    mov word ptr [di+R_EY], ax
    mov ax, word ptr [level_flip+bx]
    mov word ptr [di+R_FLIP], ax
    mov word ptr [di+R_NFLIP], 0
    test ax, ax
    jz .Lrl_ret
    mov word ptr [di+R_NFLIP], 1
.Lrl_ret:
    ret

# game_reset_player: shared by enter/restart
game_reset_player:
    mov ax, word ptr [room+R_SX]
    mov word ptr [g_px], ax
    mov ax, word ptr [room+R_SY]
    mov word ptr [g_py], ax
    xor ax, ax
    mov word ptr [g_dx], ax
    mov word ptr [g_dy], ax
    mov word ptr [g_prog], ax
    mov word ptr [g_to_exit], ax
    mov word ptr [g_keys], ax
    mov word ptr [g_bombs], ax
    mov word ptr [g_shield], ax
    mov word ptr [g_msg_t], ax
    mov word ptr [g_msg], offset str_empty
    ret

game_enter_room:                     # remember the pristine room for restarts
    mov si, offset room
    mov di, offset orig
    call copy_room
    call game_reset_player
    mov ax, word ptr [g_coins]
    mov word ptr [g_coin_rec], ax
    ret

game_restart:
    mov si, offset orig
    mov di, offset room
    call copy_room
    call game_reset_player
    mov ax, word ptr [g_coin_rec]
    mov word ptr [g_coins], ax
    ret

# game_dash: AX=dx, BX=dy.  Starts a dash unless one is running.
game_dash:
    mov cx, word ptr [g_dx]
    or cx, word ptr [g_dy]
    jnz .Lgd_no
    mov word ptr [g_dx], ax
    mov word ptr [g_dy], bx
    mov word ptr [g_prog], 0
    mov word ptr [g_to_exit], 0
    mov ax, word ptr [g_px]
    shl ax, 4
    mov word ptr [g_trail_x], ax
    mov ax, word ptr [g_py]
    shl ax, 4
    mov word ptr [g_trail_y], ax
.Lgd_no:
    ret

# game_dashing: ZF clear if a dash is running
game_dashing:
    mov ax, word ptr [g_dx]
    or ax, word ptr [g_dy]
    ret

open_gates:                          # every gate becomes floor
    mov cx, word ptr [room+R_W]
    imul cx, word ptr [room+R_H]
    xor si, si
.Log1:
    cmp byte ptr [room+R_C+si], T_GATE
    jne .Log2
    mov byte ptr [room+R_C+si], T_FLOOR
.Log2:
    inc si
    loop .Log1
    ret

flip_layout:                         # lever: toggle the listed wall cells
    xor bx, bx
.Lfl1:
    cmp bx, word ptr [room+R_NFLIP]
    jge .Lfl_ret
    mov si, bx
    shl si, 1
    mov si, word ptr [room+R_FLIP+si]
    inc bx
    test si, si
    jz .Lfl1
    cmp byte ptr [room+R_C+si], T_TOGGLE
    jne .Lfl2
    mov byte ptr [room+R_C+si], T_FLOOR
    jmp .Lfl1
.Lfl2:
    mov byte ptr [room+R_C+si], T_TOGGLE
    jmp .Lfl1
.Lfl_ret:
    ret

# begin_step: what happens when the dash tries to enter the next tile.
#   AL = 0 stop, 1 tile entered, 2 dead.  Event bits are OR-ed into [ev_acc].
begin_step:
    mov ax, word ptr [g_px]
    add ax, word ptr [g_dx]
    mov word ptr [bs_nx], ax
    mov bx, word ptr [g_py]
    add bx, word ptr [g_dy]
    mov word ptr [bs_ny], bx
    cmp ax, word ptr [room+R_EX]
    jne .Lbs_notexit
    cmp bx, word ptr [room+R_EY]
    jne .Lbs_notexit
    mov word ptr [g_to_exit], 1
    mov al, 1
    ret
.Lbs_notexit:
    test ax, ax
    js .Lbs_stop
    test bx, bx
    js .Lbs_stop
    cmp ax, word ptr [room+R_W]
    jge .Lbs_stop
    cmp bx, word ptr [room+R_H]
    jge .Lbs_stop
    mov si, bx
    imul si, word ptr [room+R_W]
    add si, ax                       # SI = cell index
    mov dl, byte ptr [room+R_C+si]
    cmp dl, T_FLOOR
    je .Lbs_go
    cmp dl, T_TRAIL
    je .Lbs_go
    cmp dl, T_COIN
    je .Lbs_coin
    cmp dl, T_CRATE
    je .Lbs_crate
    cmp dl, T_KEY
    je .Lbs_key
    cmp dl, T_BOMB
    je .Lbs_bomb
    cmp dl, T_SHIELD
    je .Lbs_shield
    cmp dl, T_LEVER
    je .Lbs_lever
    cmp dl, T_DOOR
    je .Lbs_door
    cmp dl, T_PLATE
    je .Lbs_plate
    cmp dl, T_TRAP
    je .Lbs_trap
.Lbs_stop:
    xor al, al
    ret
.Lbs_go:
    mov al, 1
    ret
.Lbs_coin:
    inc word ptr [g_coins]
    or word ptr [ev_acc], EV_COIN
    mov byte ptr [room+R_C+si], T_FLOOR
    mov al, 1
    ret
.Lbs_key:
    inc word ptr [g_keys]
    jmp .Lbs_item
.Lbs_bomb:
    inc word ptr [g_bombs]
.Lbs_item:
    or word ptr [ev_acc], EV_ITEM
    mov byte ptr [room+R_C+si], T_FLOOR
    mov al, 1
    ret
.Lbs_shield:
    or word ptr [ev_acc], EV_ITEM
    cmp word ptr [g_shield], 0
    je .Lbs_shield_new
    add word ptr [g_coins], 5
    mov word ptr [g_msg], offset str_shield_bonus
    mov word ptr [g_msg_t], 150
    jmp .Lbs_shield_done
.Lbs_shield_new:
    mov word ptr [g_shield], 1
.Lbs_shield_done:
    mov byte ptr [room+R_C+si], T_FLOOR
    mov al, 1
    ret
.Lbs_lever:
    push si
    call flip_layout
    pop si
    mov byte ptr [room+R_C+si], T_FLOOR
    or word ptr [ev_acc], EV_ITEM
    mov al, 1
    ret
.Lbs_plate:
    push si
    call open_gates
    pop si
    mov byte ptr [room+R_C+si], T_FLOOR
    or word ptr [ev_acc], EV_ITEM
    mov al, 1
    ret
.Lbs_door:
    cmp word ptr [g_keys], 0
    jle .Lbs_stop
    dec word ptr [g_keys]
    mov byte ptr [room+R_C+si], T_FLOOR
    or word ptr [ev_acc], EV_ITEM
    jmp .Lbs_stop                    # opening a door always ends the dash
.Lbs_trap:
    cmp word ptr [g_shield], 0
    jne .Lbs_trap_shield
    mov word ptr [g_msg], offset str_trap
    mov word ptr [g_msg_t], 150
    mov al, 2
    ret
.Lbs_trap_shield:
    mov word ptr [g_shield], 0
    mov byte ptr [room+R_C+si], T_FLOOR
    or word ptr [ev_acc], EV_ITEM
    mov al, 1
    ret
.Lbs_crate:
    mov bx, word ptr [bs_nx]
    add bx, word ptr [g_dx]          # tile behind the crate
    mov cx, word ptr [bs_ny]
    add cx, word ptr [g_dy]
    test bx, bx
    js .Lbs_stop
    test cx, cx
    js .Lbs_stop
    cmp bx, word ptr [room+R_W]
    jge .Lbs_stop
    cmp cx, word ptr [room+R_H]
    jge .Lbs_stop
    imul cx, word ptr [room+R_W]
    add cx, bx
    mov di, cx
    mov dl, byte ptr [room+R_C+di]
    cmp dl, T_FLOOR
    je .Lbs_push
    cmp dl, T_TRAP
    je .Lbs_push
    cmp dl, T_PLATE
    jne .Lbs_stop
.Lbs_push:
    mov byte ptr [room+R_C+di], T_CRATE
    cmp dl, T_PLATE
    jne .Lbs_push2
    push si
    call open_gates
    pop si
.Lbs_push2:
    mov byte ptr [room+R_C+si], T_FLOOR
    mov al, 1
    ret

# game_tick: AX = pixels to advance.  Returns AX = EV_* flags.
game_tick:
    mov word ptr [gt_px], ax
    mov word ptr [ev_acc], 0
.Lgt_loop:
    cmp word ptr [gt_px], 0
    jle .Lgt_done
    call game_dashing
    jz .Lgt_done
    cmp word ptr [g_prog], 0
    jne .Lgt_move
    call begin_step
    test al, al
    jz .Lgt_stop
    cmp al, 2
    je .Lgt_dead
.Lgt_move:
    mov ax, 16
    sub ax, word ptr [g_prog]
    cmp ax, word ptr [gt_px]
    jle .Lgt_s1
    mov ax, word ptr [gt_px]
.Lgt_s1:
    add word ptr [g_prog], ax
    sub word ptr [gt_px], ax
    cmp word ptr [g_prog], 16
    jl .Lgt_loop
    # Do not write trail tiles into the room: dash marks are visual-only.
.Lgt_no_trail:
    mov ax, word ptr [g_dx]
    add word ptr [g_px], ax
    mov ax, word ptr [g_dy]
    add word ptr [g_py], ax
    mov word ptr [g_prog], 0
    cmp word ptr [g_to_exit], 0
    je .Lgt_loop
    mov word ptr [g_dx], 0
    mov word ptr [g_dy], 0
    mov word ptr [g_to_exit], 0
    or word ptr [ev_acc], EV_CLEAR
    jmp .Lgt_done
.Lgt_stop:
    mov word ptr [g_dx], 0
    mov word ptr [g_dy], 0
    jmp .Lgt_done
.Lgt_dead:
    mov word ptr [g_dx], 0
    mov word ptr [g_dy], 0
    or word ptr [ev_acc], EV_DEAD
.Lgt_done:
    mov ax, word ptr [ev_acc]
    ret

# game_bomb: break weak walls next to the player.
game_bomb:
    cmp word ptr [g_bombs], 0
    jg .Lgb1
    mov word ptr [g_msg], offset str_nobombs
    mov word ptr [g_msg_t], 150
    ret
.Lgb1:
    xor si, si                       # direction index
    xor bp, bp                       # walls broken
.Lgb_loop:
    mov bx, si
    shl bx, 1
    mov ax, word ptr [g_px]
    add ax, word ptr [bomb_ox+bx]
    mov cx, word ptr [g_py]
    add cx, word ptr [bomb_oy+bx]
    test ax, ax
    js .Lgb_next
    test cx, cx
    js .Lgb_next
    cmp ax, word ptr [room+R_W]
    jge .Lgb_next
    cmp cx, word ptr [room+R_H]
    jge .Lgb_next
    imul cx, word ptr [room+R_W]
    add cx, ax
    mov bx, cx
    cmp byte ptr [room+R_C+bx], T_BREAK
    jne .Lgb_next
    mov byte ptr [room+R_C+bx], T_FLOOR
    inc bp
.Lgb_next:
    inc si
    cmp si, 4
    jl .Lgb_loop
    test bp, bp
    jnz .Lgb_ok
    mov word ptr [g_msg], offset str_nothing
    mov word ptr [g_msg_t], 150
    ret
.Lgb_ok:
    dec word ptr [g_bombs]
    ret
bomb_ox: .word 0, 0, -1, 1
bomb_oy: .word -1, 1, 0, 0
