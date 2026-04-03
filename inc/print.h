#ifndef PRINT_H
#define PRINT_H

#include <stdint.h>


int cga_init(void);
char *itoa( int value, char *str, int base);
int cga_putc(const char ch);
int cga_puts(const char *str);
int cga_printf(const char *format, ...);

#endif // PRINT_H