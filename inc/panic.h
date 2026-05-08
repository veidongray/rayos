#ifndef PANIC_H
#define PANIC_H

#include "print.h"

void panic_halt(void);
void panic_cli(void);

#define PANIC(fmt, ...)                   \
    do                                    \
    {                                     \
        cga_printf((fmt), ##__VA_ARGS__); \
        panic_cli();                      \
        panic_halt();                     \
    } while (0)

#endif // PANIC_H