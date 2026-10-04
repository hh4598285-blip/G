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
global idt_load
global irq_common_stub
extern kmain
extern irq_dispatch
extern memory_init
extern fs_init
extern system_init
extern idt_init
extern pit_init

_start:
    cli
    mov esp, stack_top
    xor ebp, ebp
    call memory_init
    call fs_init
    call system_init
    call idt_init
    push dword 100
    call pit_init
    add esp, 4
    push ebx
    push eax
    call kmain
.hang:
    cli
    hlt
    jmp .hang

idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

irq_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp
    call irq_dispatch
    add esp, 4
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iretd

%macro IRQ 2
global irq%1
irq%1:
    push dword 0
    push dword %2
    jmp irq_common_stub
%endmacro

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

section .note.GNU-stack

section .bss
align 16
stack_bottom:
    resb 32768
stack_top:
