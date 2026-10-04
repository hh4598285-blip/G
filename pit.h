#ifndef GOS_PIT_H
#define GOS_PIT_H

#include <stdint.h>
void pit_init(uint32_t frequency);
uint32_t pit_frequency(void);

#endif
