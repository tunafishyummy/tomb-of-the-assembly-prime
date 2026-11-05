# ============================================================================
#  AdLib / OPL2 music channel.
#  The PC speaker stays free for sound effects, so music and effects can play
#  at the same time instead of taking turns on one timer.
#  One OPL2 channel with a soft FM voice, attenuated under the SFX.
# ============================================================================
    .text

# opl_write: AL = register, AH = value.
opl_write:
    push ax
    push bx
    push cx
    push dx
    mov bx, ax
    mov dx, 0x388
    mov al, bl
    out dx, al
    mov cx, 6
.Lopw1:
    in al, dx
    loop .Lopw1
    inc dx
    mov al, bh
    out dx, al
    mov dx, 0x388
    mov cx, 35
.Lopw2:
    in al, dx
    loop .Lopw2
    pop dx
    pop cx
    pop bx
    pop ax
    ret

# opl_init: reset the chip and load the music voice.
opl_init:
    push ax
    mov al, 0x04
    mov ah, 0x80
    call opl_write
    mov al, 0x01
    mov ah, 0x20
    call opl_write
    mov al, 0x08
    xor ah, ah
    call opl_write
    mov al, 0xBD
    xor ah, ah
    call opl_write

    # operator 0 (modulator)
    mov al, 0x20
    mov ah, 0x01
    call opl_write
    mov al, 0x40
    mov ah, 0x20
    call opl_write
    mov al, 0x60
    mov ah, 0xF0
    call opl_write
    mov al, 0x80
    mov ah, 0x77
    call opl_write
    mov al, 0xE0
    xor ah, ah
    call opl_write

    # operator 3 (carrier)
    mov al, 0x23
    mov ah, 0x01
    call opl_write
    mov al, 0x43
    mov ah, 0x10
    call opl_write
    mov al, 0x63
    mov ah, 0xF0
    call opl_write
    mov al, 0x83
    mov ah, 0x77
    call opl_write
    mov al, 0xE3
    xor ah, ah
    call opl_write

    # channel 0: FM, no feedback
    mov al, 0xC0
    xor ah, ah
    call opl_write
    call opl_note_off
    pop ax
    ret

# opl_note_on: AX = frequency in Hz.
opl_note_on:
    push eax
    push ebx
    push ecx
    push edx
    movzx ebx, ax
    test ebx, ebx
    jz .Lon_off
    call opl_note_off
    mov eax, ebx
    shl eax, 16
    xor edx, edx
    mov ecx, 49716
    div ecx
    mov bx, ax
    mov al, 0xA0
    mov ah, bl
    call opl_write
    mov ax, bx
    shr ax, 8
    and al, 0x03
    or al, 0x30
    mov ah, al
    mov al, 0xB0
    call opl_write
    jmp .Lon_ret
.Lon_off:
    call opl_note_off
.Lon_ret:
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret

# opl_note_off: release the music channel.
opl_note_off:
    push ax
    mov al, 0xB0
    mov ah, 0x10
    call opl_write
    pop ax
    ret
