/* .COM start-up: DOS gives us CS=DS=ES=SS and SP=FFFE. */
    .code16gcc
    .section .text.start,"ax",@progbits
    .globl _start
_start:
    cld
    movzwl  %sp, %esp
    movl    $__bss_start, %edi
    movl    $__bss_end, %ecx
    subl    %edi, %ecx
    xorl    %eax, %eax
    rep stosb
    calll   game_main
    movw    $0x4C00, %ax
    int     $0x21
