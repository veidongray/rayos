#include <stdint.h>
#include "print.h"
#include "pic.h"
#include "gdt.h"
#include "idt.h"
#include "page.h"

void main(void)
{
    uint32_t len, size;

    gdt_init();
    idt_init();
    page_init();

    size = (uint32_t)_kernel_end_aligned - (uint32_t)_kernel_start;
    len = (size / 0x400000) + ((size % 0x400000) ? 1 : 0);
    cga_printf("_kernel_start: 0x%X\n", (uint32_t)_kernel_start);
    cga_printf("_kernel_end: 0x%X\n", (uint32_t)_kernel_end);
    cga_printf("_kernel_end_aligned: 0x%X\n", (uint32_t)_kernel_end_aligned);
    cga_printf("size: %u len: %u\n", size, len);
    pic_remap(0x20, 0x28);
    timer_init();
    enable_irq();

    while (1)
    {
        asm volatile("hlt\r\n");
    }
}
