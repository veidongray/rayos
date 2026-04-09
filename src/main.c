#include <stdint.h>
#include "print.h"
#include "pic.h"
#include "gdt.h"
#include "idt.h"
#include "page.h"
#include "multiboot2.h"

struct free_page {
    physaddr_t base;
    uint32_t flags;
    struct free_page *next;
} __attribute__((packed));
struct free_page *free_page_list = (struct free_page *)0;

void kernel_main(void)
{
    uint32_t i = 0;
    // gdt_init();
    idt_init();
    // page_init();

    cga_info("heap_top: 0x%X.\n", (uint32_t)heap_top - 0xc0000000);
    cga_info("Kernel initialized successfully.\n");
    cga_info("Systicks %uHZ.\n", TIMER_FREQ);
    cga_info("Total RAM: %u Bytes.\n", total_ram);
    for (i = 0; i < 0x10000000; i += 0x1000)
    {
        cga_printf("Free page: %X\n", i);
    }
    while (1)
    {
        asm volatile("hlt\r\n");
    }
}

extern uint32_t _mboot_info[];
extern uint32_t _mboot_magic[];
extern uint32_t _boot_page_directory[];

void main(void)
{
    uint32_t i;
    cga_init();
    cga_info("_mboot_info = 0x%X.\n", _mboot_info[0]);
    cga_info("_mboot_magic = 0x%X.\n", _mboot_magic[0]);
    cga_info("_boot_page_directory[%u] = 0x%X.\n", 0, _boot_page_directory[0]);
    // _boot_page_directory[0] = 0x2;
    cga_info("_boot_page_directory[%u] = 0x%X.\n", 0, _boot_page_directory[0]);
    cga_info("_boot_page_directory[%u] = 0x%X.\n", 768, _boot_page_directory[768]);
    cga_info("_boot_page_directory[%u] = 0x%X.\n", 1023, _boot_page_directory[1023]);
    cga_info("*(uint32_t)0xc0105000 = 0x%X.\n", *(uint32_t *)0xc0105000);
    cga_info("Free page: %X\n", total_ram);
    if (_mboot_magic[0] != 0x36d76289)
    {
        cga_info("Invalid multiboot magic number: 0x%X.\n", _mboot_magic[0]);
        while (1)
        {
            asm volatile("hlt\r\n");
        }
    }
    parse_multiboot2_mmap((void*)_mboot_info[0]);
    kernel_main();
}
