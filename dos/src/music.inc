# ============================================================================
#  Background music sequencer.
#  Notes go to the AdLib/OPL2 channel, so the PC speaker sound effects can play
#  at the same time without altering the music.
# ============================================================================
    .text

# music_tick: advance the theme.
music_tick:
    push ax
    cmp word ptr [mus_on], 0
    je .Lmu_tick_ret
    cmp word ptr [mus_ptr], 0
    jne .Lmu_tick_have
    mov word ptr [mus_ptr], offset mus_theme
    mov word ptr [mus_timer], 0
.Lmu_tick_have:
    dec word ptr [mus_timer]
    jg .Lmu_tick_ret
    call music_step
.Lmu_tick_ret:
    pop ax
    ret

# music_step: play the next note or rest, and loop at the 0xFFFF marker.
music_step:
    push ax
    push cx
    push si
    mov si, word ptr [mus_ptr]
    test si, si
    jz .Lmu_step_init
    mov ax, word ptr [si]
    cmp ax, 0xFFFF
    jne .Lmu_step_note
    mov si, offset mus_theme
    mov word ptr [mus_ptr], si
    mov ax, word ptr [si]
.Lmu_step_note:
    mov cx, word ptr [si+2]
    mov word ptr [mus_timer], cx
    add word ptr [mus_ptr], 4
    test ax, ax
    jz .Lmu_step_rest
    call opl_note_on
    jmp .Lmu_step_ret
.Lmu_step_rest:
    call opl_note_off
    jmp .Lmu_step_ret
.Lmu_step_init:
    mov word ptr [mus_ptr], offset mus_theme
    mov word ptr [mus_timer], 0
.Lmu_step_ret:
    pop si
    pop cx
    pop ax
    ret

# sound_tick: called once per frame.  SFX and music use separate hardware.
sound_tick:
    push ax
    call sfx_tick
    call music_tick
    pop ax
    ret

# --- theme ------------------------------------------------------------------
# Slow D-minor tomb motif.  Word pairs: frequency in Hz, duration in frames.
# 0xFFFF loops back to the start and 0 is a rest.
mus_theme:
    .word 294, 18
    .word 349, 18
    .word 440, 36
    .word 392, 18
    .word 349, 18
    .word 294, 36
    .word 262, 18
    .word 294, 18
    .word 349, 36
    .word 330, 18
    .word 294, 36
    .word 0, 18

    .word 440, 18
    .word 466, 18
    .word 523, 36
    .word 466, 18
    .word 440, 18
    .word 392, 36
    .word 349, 18
    .word 392, 18
    .word 440, 36
    .word 392, 18
    .word 349, 36
    .word 0, 18

    .word 220, 18
    .word 262, 18
    .word 294, 36
    .word 262, 18
    .word 220, 18
    .word 196, 36
    .word 175, 18
    .word 196, 18
    .word 220, 36
    .word 262, 18
    .word 294, 36
    .word 147, 36
    .word 0, 18

    .word 0xFFFF, 0
