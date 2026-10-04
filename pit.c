#include "pit.h"
#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40
static uint32_t current_frequency;
static inline void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
void pit_init(uint32_t frequency){
    if(frequency<20) frequency=20;
    if(frequency>1193182) frequency=1193182;
    uint32_t divisor=1193182/frequency;
    if(divisor==0) divisor=1;
    current_frequency=1193182/divisor;
    outb(PIT_COMMAND,0x36);
    outb(PIT_CHANNEL0,(uint8_t)(divisor&0xFF));
    outb(PIT_CHANNEL0,(uint8_t)((divisor>>8)&0xFF));
}
uint32_t pit_frequency(void){return current_frequency;}
