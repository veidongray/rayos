#include "cursor.h"

void set_cursor(ushort x, ushort y)
{
    ushort cursor_pos = (y * 80) + x;

    outb(0x03d4, 0xe);
    outb(0x03d5, (cursor_pos >> 8) & 0xff);
    outb(0x03d4, 0xf);
    outb(0x03d5, cursor_pos & 0xff);
}

ushort get_cursor(void)
{
    uchar cursor_pos_h;
    uchar cursor_pos_l;

    outb(0x03d4, 0xe);
    cursor_pos_h = inb(0x03d5);
    outb(0x03d4, 0xf);
    cursor_pos_l = inb(0x03d5);

    return (cursor_pos_h << 8) | cursor_pos_l;
}