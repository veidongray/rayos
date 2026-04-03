#include "systicks.h"

static uint32_t systicks = 0;
uint32_t get_systicks(void)
{
    return systicks;
}
void set_systicks(uint32_t ticks)
{
    systicks = ticks;
}