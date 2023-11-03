#include "x86.h"
#include "mmu.h"

void init_int(void)
{
    set_idt();
    init_8259a();
    lidt(0x20000, 0x800);
    init_keyboard();
    sti();
}

void set_8259a_irq_mask(ushort mask)
{
    outb(0x21, mask & 0xff);
    outb(0xa1, (mask >> 8) & 0xff);
}