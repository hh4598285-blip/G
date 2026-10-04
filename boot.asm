BITS 32

section .multiboot
align 4
multiboot_header:
    dd 0x1BADB002
    dd 0x00000007
    dd -(0x1BADB002 + 0x00000007)
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0

section .text
align 16
global _start
extern kmain

_start:
    cli
    mov esp, stack_top
    xor ebp, ebp
    push ebx
    push eax
    call kmain

.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
