#ifndef PRINT_H
#define PRINT_H

#include <stdint.h>
#include "systicks.h"

#define cga_info(format, ...) \
do { \
    cga_printf("[%u] " format, get_systicks(), ##__VA_ARGS__); \
} while(0)

int cga_init(void);
char *itoa(int value, char *str, int base);
char *uitoa(uint32_t value, char *str, int base);
int cga_putc(const char ch);
int cga_puts(const char *str);
int cga_printf(const char *format, ...);

#endif // PRINT_H