#include <int.h>
#include <gdt.h>
#include <pic.h>
#include <page.h>
#include <lapic.h>
#include <multiboot2.h>

void start_kernel(void)
{
    total_memory_init();
    gdt_init();
    page_init();
    int_init();
    lapic_init();
    enable_irq();
    while (1)
    {
        asm volatile("hlt");
    }
}
