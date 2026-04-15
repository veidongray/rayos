#include "multiboot2.h"
#include "print.h"
#include "gdt.h"
#include "irq.h"
#include <stdint.h>

extern uint32_t _mboot_info[];
extern uint32_t _mboot_magic[];

void main(void)
{
    if (_mboot_magic[0] == 0x36d76289)
        parse_multiboot2_mmap((uint32_t *)_mboot_info[0]);
    else
    {
        cga_printf("Haven't Bootloader...\n");
        while (1) asm volatile("hlt\r\n");
    }
    gdt_init();
    idt_init();
    while (1) asm volatile("hlt\r\n");
}
