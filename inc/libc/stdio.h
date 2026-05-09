#ifndef STDIO_H
#define STDIO_H

#include <stdint.h>

#define STDIN 0x0
#define STDOUT 0x1
#define STDERR 0x2

int printf(const char *fmt, ...);

#endif // STDIO_H