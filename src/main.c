#include "multiboot2.h"
#include "print.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include <stdint.h>

extern uint32_t _mboot_info[];
extern uint32_t _mboot_magic[];
extern uint32_t host_total_mem;

void main(void)
{
    if (_mboot_magic[0] == 0x36d76289)
        parse_multiboot2_mmap((uint32_t *)_mboot_info[0]);
    else
    {
        // If we don't have a valid multiboot magic number, we can't trust the bootloader and should halt
        cga_printf("Lost Bootloader...\n");
        while (1) asm volatile("hlt\r\n");
    }

    // Check if we have at least 4MB of memory, otherwise we can't do much
    if (host_total_mem < 0x400000)
    {
        cga_printf("Not enough memory detected: %u bytes\n", host_total_mem);
        while (1) asm volatile("hlt\r\n");
    }
    gdt_init();
    idt_init();
    page_init();
    while (1) asm volatile("hlt\r\n");
}
