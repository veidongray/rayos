#ifndef __X86_H__
#define __X86_H__

#include "types.h"

static inline uchar
inb(ushort port)
{
    uchar data;

    asm volatile("in %1,%0" : "=a"(data) : "d"(port));
    return data;
}

static inline void
outb(ushort port, uchar data)
{
    asm volatile("out %0,%1" : : "a"(data), "d"(port));
}

#endif //__X86_H__