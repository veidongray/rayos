#include <stdint.h>
#include "multiboot2.h"
#include "paging.h"
#include "print.h"

extern uint32_t _virt_offset[];
extern uint32_t _kernel_base[];
extern uint32_t _kernel_virt_start[];
extern uint32_t _kernel_phys_start[];
extern uint32_t _kernel_virt_end[];
extern uint32_t _kernel_phys_end[];
extern uint32_t _kernel_virt_end_aligned[];
extern uint32_t _kernel_phys_end_aligned[];
extern uint32_t host_total_mem;

extern void load_page_directory(uint32_t *page_directory);
extern void enable_paging();

static uint32_t kpage_directory[1024] __attribute__((aligned(4096)));
static uint32_t *kpage_tables;

int page_init(void)
{
    uint32_t i;
    // Calculate the total number of pages needed for the host memory
    uint32_t ktotal_pages =
        (host_total_mem / 0x1000) + ((host_total_mem % 0x1000) ? 1 : 0); // Round up to nearest page
    uint32_t ktotal_page_tables =
        (ktotal_pages / 0x400) + ((ktotal_pages % 0x400) ? 1 : 0); // Each page table can map 1024 pages
    cga_printf("Total Memory: %u bytes, Total Pages: %u, Total Page Tables: %u\n",
        host_total_mem, ktotal_pages, ktotal_page_tables);

    uint32_t kernel_size =
        (uint32_t)_kernel_virt_end_aligned
        - (uint32_t)_kernel_virt_start + (uint32_t)0x400
        + (uint32_t)ktotal_page_tables * 0x400;
    cga_printf("Kernel Size: %u bytes\n", kernel_size);

    kpage_tables = (uint32_t *)((uint32_t)_kernel_virt_end_aligned); // Place page tables right after the kernel
    // Map all memory up to the total memory size, including the kernel itself
    for (i = 0; i < ktotal_pages; i++)
        kpage_tables[i] = (i * 0x1000) | 0x3; // Present + Read/Write
    for (i = 0; i < ktotal_page_tables; i++)
        kpage_directory[i + 0x300] =
        (uint32_t)((uint32_t)kpage_tables - (uint32_t)_virt_offset + (i * 0x1000)) | 0x3; // Present + Read/Write
    kpage_directory[1023] = (uint32_t)((uint32_t)kpage_directory - (uint32_t)_virt_offset) | 0x3; // Map the last page to itself for recursive paging
    load_page_directory((uint32_t *)((uint32_t)kpage_directory - (uint32_t)_virt_offset));
    enable_paging();
    return 0;
}