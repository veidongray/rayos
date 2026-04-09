#include <stdint.h>
#include "print.h"
#include "pic.h"
#include "gdt.h"
#include "idt.h"
#include "page.h"
#include "multiboot2.h"

void kernel_main(void)
{
    gdt_init();
    idt_init();
    page_init();

    cga_info("heap_top: 0x%X.\n", (uint32_t)heap_top);
    cga_info("Kernel initialized successfully.\n");
    cga_info("Systicks %uHZ.\n", TIMER_FREQ);
    while (1)
    {
        asm volatile("hlt\r\n");
    }
}

void main(uint32_t grub_magic, uint32_t mbi_addr)
{
    cga_init();
    if (grub_magic != 0x36d76289)
    {
        cga_info("Invalid multiboot magic number: 0x%X.\n", grub_magic);
        while (1)
        {
            asm volatile("hlt\r\n");
        }
    }
    parse_multiboot2_mmap((void*)mbi_addr);
    while(1);
    kernel_main();
}
