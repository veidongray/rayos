#include <stdint.h>
#include "print.h"
#include "pic.h"
#include "gdt.h"
#include "idt.h"
#include "page.h"

void main(void)
{
    cga_init();
    gdt_init();
    idt_init();
    page_init();

    cga_info("kernel_end: 0x%X.\n", (uint32_t)kernel_end);
    cga_info("Kernel initialized successfully.\n");
    cga_info("Systicks %uHZ.\n", TIMER_FREQ);
    while (1)
    {
        asm volatile("hlt\r\n");
    }
}
