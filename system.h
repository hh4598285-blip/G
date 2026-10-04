#ifndef GOS_SYSTEM_H
#define GOS_SYSTEM_H

#include <stdint.h>

void system_init(void);
void system_poll(void);
void system_print(const char* text);
void system_print_hex(uint32_t value);
uint32_t system_ticks(void);

#endif
