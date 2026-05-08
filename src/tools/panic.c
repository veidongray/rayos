#include "panic.h"

void panic_halt(void)
{
    asm volatile("hlt");
}

void panic_cli(void)
{
    asm volatile("cli");
}