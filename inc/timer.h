#ifndef TIMER_H
#define TIMER_H

#include <types.h>

uint64_t get_uptime_ms(void);
void mdelay(uint32_t ms);
void sdelay(uint32_t s);

#endif /* TIMER_H */