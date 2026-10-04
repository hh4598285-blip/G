BITS 32

section .multiboot
align 4
multiboot_header:
    dd 0x1BADB002
    dd 0x00000007
    dd -(0x1BADB002 + 0x00000007)
    dd 0
    dd 1024
    dd 768
    dd 32

section .text
align 16
global _start
extern kmain
extern system_init
extern memory_init
extern fs_init

_start:
    cli
    mov esp, stack_top
    xor ebp, ebp

    call memory_init
    call fs_init
    call system_init

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
    resb 32768
stack_top:
