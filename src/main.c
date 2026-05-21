#include <gdt.h>
#include <page.h>
#include <multiboot2.h>

void start_kernel(void)
{
    total_memory_init();
    gdt_init();
    page_init();
    while (1)
    {
        asm volatile("hlt");
    }
}
