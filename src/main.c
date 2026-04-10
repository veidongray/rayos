#include <stdint.h>
#include "print.h"
#include "pic.h"
#include "gdt.h"
#include "idt.h"
#include "page.h"
#include "multiboot2.h"

extern uint32_t _mboot_info[];
extern uint32_t _mboot_magic[];
extern uint32_t _boot_page_directory[];
extern uint32_t _kernel_end_aligned[];

struct page {
    physaddr_t base;
    uint32_t flags;
    uint32_t kref;
};

struct free_page {
    struct page page;
    struct free_page *next;
};
uint32_t free_page_count = 0;
struct free_page *free_page_list = (struct free_page *)0;

void kernel_main(void)
{
    uint32_t i = 0;
    gdt_init();
    idt_init();
    // page_init();

    cga_init();
    cga_info("_mboot_info = 0x%X.\n", _mboot_info[0]);
    cga_info("_mboot_magic = 0x%X.\n", _mboot_magic[0]);
    cga_info("_boot_page_directory[%u] = 0x%X.\n", 0, _boot_page_directory[0]);
    cga_info("heap_top: 0x%X.\n", (uint32_t)heap_top - 0xc0000000);
    cga_info("_boot_page_directory[%u] = 0x%X.\n", 0, _boot_page_directory[0]);
    cga_info("_boot_page_directory[%u] = 0x%X.\n", 768, _boot_page_directory[768]);
    cga_info("_boot_page_directory[%u] = 0x%X.\n", 1023, _boot_page_directory[1023]);
    cga_info("*(uint32_t)0xc0105000 = 0x%X.\n", *(uint32_t *)0xc0105000);
    cga_info("total_ram: %X\n", total_ram);
    cga_info("Kernel initialized successfully.\n");
    cga_info("Systicks %uHZ.\n", TIMER_FREQ);
    cga_info("Total RAM: %u Bytes.\n", total_ram);

    free_page_list = (struct free_page *)((uint32_t)heap_top + 0xC0000000);
    free_page_count = ((uint32_t)total_ram - (uint32_t)_kernel_end_aligned) >> 12;
    free_page_count += free_page_count % 2 ? 1 : 0; // round up
    cga_info("Free page count: %u.\n", free_page_count);
    heap_top = (physaddr_t)((uint32_t)heap_top + (free_page_count * sizeof(struct free_page)));
    heap_top = (uint32_t)heap_top % 0x1000 ? ((uint32_t)heap_top + 0x1000) & ~0xfff : heap_top; // align to page boundary
    cga_info("Free page list starts at: 0x%X.\n", (uint32_t)free_page_list);

    for (i = 0; i < free_page_count; ++i)
    {
        free_page_list[i].page.base = (physaddr_t)((uint32_t)_kernel_end_aligned + (i * 0x1000));
        free_page_list[i].page.flags = 0;
    }
    for (i = 0; i < 32; i++)
    {
        cga_info("Alloc page %u: 0x%X.\n", i, alloc_page());
    }
    
    while (1)
    {
        asm volatile("hlt\r\n");
    }
}

physaddr_t alloc_page(void)
{
    uint32_t i;
    extern struct free_page *free_page_list;
    physaddr_t page = (physaddr_t)-1;

    page = free_page_list[0].page.base;
    free_page_list = &free_page_list[1];
    return page;
}

void main(void)
{
    _boot_page_directory[0] = 0x2;
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
