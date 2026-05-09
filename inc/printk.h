#ifndef PRINTK_H
#define PRINTK_H

#include "tty.h"
#include "idt.h"

#define printk(fmt, ...)                      \
    do                                        \
    {                                         \
        if (is_interrupts_enabled())          \
        {                                     \
            disable_irq();                    \
            cga_printf((fmt), ##__VA_ARGS__); \
            enable_irq();                     \
        }                                     \
        else                                  \
        {                                     \
            cga_printf((fmt), ##__VA_ARGS__);  \
        }                                     \
    } while (0)

#endif // PRINTK_H