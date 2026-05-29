#ifndef PRINTK_H
#define PRINTK_H

#include <lib/printf/printf.h>

#define printk(fmt, ...)            \
    do                              \
    {                               \
        printf("%s[%llu]: " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#endif // PRINTK_H