#include "apic.h"
#include "panic.h"
#include "cpuid.h"
#include "paging.h"
#include "io.h"

void apic_init(void)
{
    if (!is_apic_supported())
    {
        PANIC("APIC\n");
    }

    // map 0xfee00000
    map_page((uint32_t *)0xfee00000, (uint32_t *)0xfee00000, 0x1b);

    // mask PIC
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    lapic_write(LAPIC_SVR, 0x1FF);

    // setup APIC timer
    lapic_write(LAPIC_TICFG, 0x3);
    lapic_write(LAPIC_LVT_TMR, 0x20000 | 32);
    lapic_write(LAPIC_TIC, 10000000);

    lapic_write(LAPIC_LVT_LINT0, 0x10000); // Masked
    lapic_write(LAPIC_LVT_LINT1, 0x10000); // Masked

    lapic_write(LAPIC_LVT_ERR, 33); // 错误中断向量号 33

    lapic_write(LAPIC_ESR, 0);
    lapic_write(LAPIC_ESR, 0); // 连续写两次是Intel手册建议的

    lapic_write(LAPIC_EOI, 0);
    lapic_write(LAPIC_TPR, 0);
}