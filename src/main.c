#include "gdt.h"
#include "multiboot2.h"

void start_kernel(void)
{
    total_memory_init();
    gdt_init();
    while (1)
        ;
}
