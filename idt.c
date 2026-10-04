#include "idt.h"

#define IDT_COUNT 256
#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)
#define PIC_EOI 0x20

static idt_entry_t idt[IDT_COUNT];
static idt_ptr_t idtp;
static volatile uint32_t ticks;

extern void idt_load(idt_ptr_t* ptr);
extern void irq0(void); extern void irq1(void); extern void irq2(void); extern void irq3(void);
extern void irq4(void); extern void irq5(void); extern void irq6(void); extern void irq7(void);
extern void irq8(void); extern void irq9(void); extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void); extern void irq15(void);

static inline void outb(uint16_t port, uint8_t value) { __asm__ volatile("outb %0,%1" : : "a"(value), "Nd"(port)); }
static inline uint8_t inb(uint16_t port) { uint8_t v; __asm__ volatile("inb %1,%0" : "=a"(v) : "Nd"(port)); return v; }

static void set_gate(uint8_t n, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[n].base_low = (uint16_t)(base & 0xFFFFu);
    idt[n].selector = selector;
    idt[n].zero = 0;
    idt[n].flags = flags;
    idt[n].base_high = (uint16_t)((base >> 16) & 0xFFFFu);
}

static void pic_remap(void) {
    uint8_t a1 = inb(PIC1_DATA), a2 = inb(PIC2_DATA);
    outb(PIC1_COMMAND, 0x11); for(volatile int i=0;i<100;i++);
    outb(PIC2_COMMAND, 0x11); for(volatile int i=0;i<100;i++);
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void idt_init(void) {
    for (uint32_t i=0;i<IDT_COUNT;i++) set_gate((uint8_t)i, 0, 0x08, 0x8E);
    set_gate(32,(uint32_t)(uintptr_t)irq0,0x08,0x8E); set_gate(33,(uint32_t)(uintptr_t)irq1,0x08,0x8E);
    set_gate(34,(uint32_t)(uintptr_t)irq2,0x08,0x8E); set_gate(35,(uint32_t)(uintptr_t)irq3,0x08,0x8E);
    set_gate(36,(uint32_t)(uintptr_t)irq4,0x08,0x8E); set_gate(37,(uint32_t)(uintptr_t)irq5,0x08,0x8E);
    set_gate(38,(uint32_t)(uintptr_t)irq6,0x08,0x8E); set_gate(39,(uint32_t)(uintptr_t)irq7,0x08,0x8E);
    set_gate(40,(uint32_t)(uintptr_t)irq8,0x08,0x8E); set_gate(41,(uint32_t)(uintptr_t)irq9,0x08,0x8E);
    set_gate(42,(uint32_t)(uintptr_t)irq10,0x08,0x8E); set_gate(43,(uint32_t)(uintptr_t)irq11,0x08,0x8E);
    set_gate(44,(uint32_t)(uintptr_t)irq12,0x08,0x8E); set_gate(45,(uint32_t)(uintptr_t)irq13,0x08,0x8E);
    set_gate(46,(uint32_t)(uintptr_t)irq14,0x08,0x8E); set_gate(47,(uint32_t)(uintptr_t)irq15,0x08,0x8E);
    pic_remap();
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)(uintptr_t)&idt[0];
    idt_load(&idtp);
    __asm__ volatile("sti");
}

void irq_dispatch(uint32_t* frame) {
    uint32_t vector = frame[1];
    if (vector == 32) ticks++;
    if (vector >= 40) outb(PIC2_COMMAND, PIC_EOI);
    if (vector >= 32 && vector <= 47) outb(PIC1_COMMAND, PIC_EOI);
}

uint32_t irq_ticks(void) { return ticks; }
