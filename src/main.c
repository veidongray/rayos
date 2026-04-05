#include <stdint.h>
#include "print.h"
#include "pic.h"
#include "gdt.h"
#include "idt.h"
#include "page.h"

void main(void)
{
    gdt_init();
    idt_init();
    page_init();
    pic_remap(0x20, 0x28);
    timer_init();
    enable_irq();
    
    cga_printf("_kernel_start: 0x%X\n", (uint32_t)_kernel_start);
    cga_printf("_kernel_end: 0x%X\n", (uint32_t)_kernel_end);
    cga_printf("_kernel_end_aligned: 0x%X\n", (uint32_t)_kernel_end_aligned);
    while (1)
    {
        asm volatile("hlt\r\n");
    }
}
