#ifndef SYSTICKS_H
#define SYSTICKS_H

#include <stdint.h>

uint32_t get_systicks(void);
void set_systicks(uint32_t ticks);

#endif // SYSTICKS_H