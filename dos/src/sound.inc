# ============================================================================
#  PC speaker sound effects (PIT channel 2).
#  Sequences are non-blocking: sfx_start begins one, sfx_tick advances it once
#  per rendered frame.  A sequence is a list of word pairs (frequency in Hz,
#  duration in frames) terminated by a zero frequency.
# ============================================================================
    .text

# speaker_off: close the channel 2 gate, leaving the other port B bits alone.
speaker_off:
    push ax
    in al, 0x61
    and al, 0xFC
    out 0x61, al
    pop ax
    ret

# speaker_tone: AX = frequency in Hz.  Programs channel 2 as a square wave.
speaker_tone:
    push eax
    push ebx
    push edx
    movzx ebx, ax
    test ebx, ebx
    jz .Lst_off
    mov eax, 1193182                 # PIT input clock
    xor edx, edx
    div ebx                          # EAX = divisor
    mov bx, ax
    mov al, 0xB6                     # channel 2, mode 3, binary, LSB then MSB
    out 0x43, al
    mov al, bl
    out 0x42, al
    mov al, bh
    out 0x42, al
    in al, 0x61
    or al, 3                         # enable the gate and the speaker data
    out 0x61, al
    jmp .Lst_ret
.Lst_off:
    call speaker_off
.Lst_ret:
    pop edx
    pop ebx
    pop eax
    ret

# sfx_start: AX = sequence address.  Ignored while SOUND FX is off.
sfx_start:
    push ax
    push cx
    push si
    cmp word ptr [sfx_on], 0
    je .Lsfxs_ret
    mov word ptr [sfx_ptr], ax
    mov word ptr [sfx_timer], 0
    call sfx_step
.Lsfxs_ret:
    pop si
    pop cx
    pop ax
    ret

# sfx_stop: silence the speaker and cancel the running sequence.
sfx_stop:
    push ax
    mov word ptr [sfx_ptr], 0
    mov word ptr [sfx_timer], 0
    call speaker_off
    pop ax
    ret

# sfx_tick: advance the current note.  Called once per rendered frame.
sfx_tick:
    push ax
    push si
    cmp word ptr [sfx_ptr], 0
    je .Lsfxt_ret
    cmp word ptr [sfx_on], 0
    jne .Lsfxt_on
    call sfx_stop
    jmp .Lsfxt_ret
.Lsfxt_on:
    dec word ptr [sfx_timer]
    jg .Lsfxt_ret
    call sfx_step
.Lsfxt_ret:
    pop si
    pop ax
    ret

# sfx_step: program the current note, or stop at the end of the sequence.
sfx_step:
    push ax
    push cx
    push si
    mov si, word ptr [sfx_ptr]
    test si, si
    jz .Lsfxstep_off
    mov ax, word ptr [si]
    test ax, ax
    jz .Lsfxstep_done
    mov cx, word ptr [si+2]
    mov word ptr [sfx_timer], cx
    add word ptr [sfx_ptr], 4
    call speaker_tone
    jmp .Lsfxstep_ret
.Lsfxstep_done:
    mov word ptr [sfx_ptr], 0
.Lsfxstep_off:
    call speaker_off
.Lsfxstep_ret:
    pop si
    pop cx
    pop ax
    ret

# --- sound effects -----------------------------------------------------------
# The tomb theme: low stone thuds for mechanisms and bright chimes for loot.
# Each sfx_* entry below is a callable wrapper for one sequence.
seq_menu_move:
    .word 330, 2
    .word 0, 0
seq_menu_ok:
    .word 523, 2
    .word 784, 3
    .word 0, 0
seq_back:
    .word 392, 2
    .word 262, 2
    .word 0, 0
seq_deny:
    .word 196, 3
    .word 147, 4
    .word 0, 0
seq_coin:
    .word 1047, 2
    .word 1568, 3
    .word 0, 0
seq_item:
    .word 659, 2
    .word 988, 2
    .word 1319, 3
    .word 0, 0
seq_shield:
    .word 523, 2
    .word 659, 2
    .word 784, 2
    .word 1047, 5
    .word 0, 0
seq_shield_hit:
    .word 1568, 2
    .word 1047, 2
    .word 784, 3
    .word 0, 0
seq_lever:
    .word 262, 2
    .word 196, 2
    .word 147, 3
    .word 0, 0
seq_plate:
    .word 196, 2
    .word 262, 2
    .word 330, 3
    .word 0, 0
seq_door:
    .word 220, 3
    .word 165, 3
    .word 131, 4
    .word 0, 0
seq_bomb:
    .word 147, 2
    .word 123, 2
    .word 98, 2
    .word 82, 2
    .word 65, 3
    .word 55, 4
    .word 0, 0
seq_crate:
    .word 110, 2
    .word 82, 2
    .word 0, 0
seq_death:
    .word 392, 3
    .word 311, 3
    .word 233, 4
    .word 175, 5
    .word 131, 6
    .word 0, 0
seq_clear:
    .word 523, 3
    .word 659, 3
    .word 784, 3
    .word 1047, 6
    .word 0, 0
seq_win:
    .word 523, 3
    .word 659, 3
    .word 784, 3
    .word 1047, 3
    .word 1319, 8
    .word 0, 0
seq_gameover:
    .word 392, 4
    .word 330, 4
    .word 262, 5
    .word 196, 7
    .word 0, 0

# --- callable entries --------------------------------------------------------
sfx_menu_move:
    push ax
    mov ax, offset seq_menu_move
    call sfx_start
    pop ax
    ret
sfx_menu_ok:
    push ax
    mov ax, offset seq_menu_ok
    call sfx_start
    pop ax
    ret
sfx_back:
    push ax
    mov ax, offset seq_back
    call sfx_start
    pop ax
    ret
sfx_deny:
    push ax
    mov ax, offset seq_deny
    call sfx_start
    pop ax
    ret
sfx_coin:
    push ax
    mov ax, offset seq_coin
    call sfx_start
    pop ax
    ret
sfx_item:
    push ax
    mov ax, offset seq_item
    call sfx_start
    pop ax
    ret
sfx_shield:
    push ax
    mov ax, offset seq_shield
    call sfx_start
    pop ax
    ret
sfx_shield_hit:
    push ax
    mov ax, offset seq_shield_hit
    call sfx_start
    pop ax
    ret
sfx_lever:
    push ax
    mov ax, offset seq_lever
    call sfx_start
    pop ax
    ret
sfx_plate:
    push ax
    mov ax, offset seq_plate
    call sfx_start
    pop ax
    ret
sfx_door:
    push ax
    mov ax, offset seq_door
    call sfx_start
    pop ax
    ret
sfx_bomb:
    push ax
    mov ax, offset seq_bomb
    call sfx_start
    pop ax
    ret
sfx_crate:
    push ax
    mov ax, offset seq_crate
    call sfx_start
    pop ax
    ret
sfx_death:
    push ax
    mov ax, offset seq_death
    call sfx_start
    pop ax
    ret
sfx_clear:
    push ax
    mov ax, offset seq_clear
    call sfx_start
    pop ax
    ret
sfx_win:
    push ax
    mov ax, offset seq_win
    call sfx_start
    pop ax
    ret
sfx_gameover:
    push ax
    mov ax, offset seq_gameover
    call sfx_start
    pop ax
    ret
