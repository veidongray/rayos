#include "mmu.h"
#include "x86.h"

void init_gdt(void)
{
    struct segdesc *gdt = (struct segdesc *)0x10000;

    gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, 0);
    gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);

    lgdt(gdt, 0x10000);
}