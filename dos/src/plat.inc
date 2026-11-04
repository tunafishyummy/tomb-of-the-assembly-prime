# ============================================================================
#  Platform layer: VGA mode 13h, double buffer, vsync, BIOS keyboard, timer.
#  ES always holds the back-buffer segment while the game runs.
# ============================================================================
    .text
# plat_init: set mode 13h, load the palette, find the back buffer (the 64K behind us)
plat_init:
    mov ax, cs
    add ax, 0x1000
    mov word ptr [backseg], ax
    add ax, 0x0FA0                   # 64000 bytes = 4000 paragraphs
    mov bx, word ptr [0x0002]        # PSP:0002 = first segment past our memory block
    cmp ax, bx
    jbe .Lpi_mem_ok
    mov dx, offset msg_nomem
    mov ah, 9
    int 0x21
    mov ax, 0x4C01
    int 0x21
.Lpi_mem_ok:
    mov ax, 0x0013
    int 0x10                         # 320x200x256
    mov ax, 0x1012                   # set block of DAC registers (ES:DX = table, ES = DS here)
    xor bx, bx
    mov cx, P_COUNT
    mov dx, offset palette
    int 0x10
    mov es, word ptr [backseg]
    xor di, di
    xor eax, eax
    mov cx, 16000
    cld
    rep stosd
    ret

plat_quit:                           # back to text mode
    mov ax, 0x0003
    int 0x10
    ret

# plat_present: copy the back buffer to the screen
plat_present:
    push ds
    push es
    mov ax, word ptr [backseg]
    mov ds, ax
    mov ax, 0xA000
    mov es, ax
    xor si, si
    xor di, di
    mov cx, 16000
    cld
    rep movsd
    pop es
    pop ds
    ret

# plat_wait_frame: wait for the start of the next vertical retrace (70 Hz)
plat_wait_frame:
    mov dx, 0x3DA
.Lpw1:
    in al, dx
    test al, 8
    jnz .Lpw1
.Lpw2:
    in al, dx
    test al, 8
    jz .Lpw2
    ret

# plat_key: AX = next key (K_xxx or ASCII), 0 if none. Never blocks.
plat_key:
    mov ah, 1
    int 0x16
    jz .Lpk_none
    xor ax, ax
    int 0x16
    test al, al
    jz .Lpk_ext
    cmp al, 0xE0
    je .Lpk_ext
    xor ah, ah
    ret
.Lpk_ext:
    cmp ah, 0x48
    jne .Lpk1
    mov ax, K_UP
    ret
.Lpk1:
    cmp ah, 0x50
    jne .Lpk2
    mov ax, K_DOWN
    ret
.Lpk2:
    cmp ah, 0x4B
    jne .Lpk3
    mov ax, K_LEFT
    ret
.Lpk3:
    cmp ah, 0x4D
    jne .Lpk_none
    mov ax, K_RIGHT
    ret
.Lpk_none:
    xor ax, ax
    ret

plat_flush_keys:
.Lpf1:
    mov ah, 1
    int 0x16
    jz .Lpf2
    xor ax, ax
    int 0x16
    jmp .Lpf1
.Lpf2:
    ret

# plat_ticks: EAX = BIOS tick counter
plat_ticks:
    xor ax, ax
    int 0x1A
    mov ax, cx
    shl eax, 16
    mov ax, dx
    ret

# seed_mix: mix a key code (AX) and the clock into the random seed
seed_mix:
    push eax
    push ebx
    movzx ebx, ax                    # keep the key in EBX: INT 1Ah (plat_ticks) overwrites CX:DX
    call plat_ticks
    add ebx, eax
    mov eax, dword ptr [seed]
    imul eax, eax, 31
    add eax, ebx
    mov dword ptr [seed], eax
    pop ebx
    pop eax
    ret

# rng_next: EAX = next 15-bit random number (same LCG as the C reference)
rng_next:
    push edx
    mov eax, dword ptr [seed]
    mov edx, 1664525
    mul edx
    add eax, 1013904223
    mov dword ptr [seed], eax
    shr eax, 16
    and eax, 0x7FFF
    pop edx
    ret

# rnd: AX = random number in 0..CX-1   (preserves everything else)
rnd:
    push edx
    push ecx
    call rng_next
    xor edx, edx
    movzx ecx, cx
    div ecx
    mov ax, dx
    pop ecx
    pop edx
    ret
