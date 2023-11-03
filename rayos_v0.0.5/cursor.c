#include "x86.h"

void set_cursor(const ushort pos)
{
    outb(0x03d4, 0xe);
    outb(0x03d5, (pos >> 8) & 0xff);
    outb(0x03d4, 0xf);
    outb(0x03d5, pos & 0xff);
}

ushort get_cursor(void)
{
    ushort pos;

    outb(0x03d4, 0xe);
    pos = inb(0x03d5);
    pos <<= 8;
    outb(0x03d4, 0xf);
    pos = inb(0x03d5);
    return pos;
}