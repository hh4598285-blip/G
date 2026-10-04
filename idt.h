#ifndef GOS_IDT_H
#define GOS_IDT_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} idt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint32_t base;
} idt_ptr_t;

void idt_init(void);
void irq_dispatch(uint32_t* frame);
uint32_t irq_ticks(void);

#endif
