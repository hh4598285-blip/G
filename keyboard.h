#ifndef GOS_KEYBOARD_H
#define GOS_KEYBOARD_H
#include <stdint.h>
void keyboard_init(void);
void keyboard_irq(void);
int keyboard_read(char* out);
uint32_t keyboard_key_count(void);
#endif
