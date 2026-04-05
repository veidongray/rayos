#include "page.h"
#include "print.h"

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

int page_init(void)
{
    int i, j;

    for (i = 0; i < 1024; i++)
    {
        // This sets the following flags to the pages:
        //   Supervisor: Only kernel-mode can access them
        //   Write Enabled: It can be both read from and written to
        //   Not Present: The page table is not present
        page_directory[i] = 0x00000002;
    }
    // we will fill all 1024 entries in the table, mapping 4 megabytes
    for (i = 0; i < 1024; i++)
    {
        // As the address is page aligned, it will always leave 12 bits zeroed.
        // Those bits are used by the attributes ;)
        first_page_table[i] = (i * 0x1000) | 0x00000003; // attributes: supervisor level, read/write, present.
    }
    // attributes: supervisor level, read/write, present
    page_directory[0] = ((uint32_t)first_page_table) | 0x00000003;
    // 0xC0000000 map to 0x00000000
    page_directory[768] = ((uint32_t)first_page_table) | 0x00000003;
    // self-mapping
    page_directory[1023] = ((uint32_t)page_directory) | 0x00000003;
    /* Map:
     * 0x00000000 ~ 0x00400000 to 0x00000000 ~ 0x00400000
     * 0xC0000000 ~ 0xC0400000 to 0xC0000000 ~ 0xC0400000
     * 0xFFFFF000 to page_directory
     * ((uint32_t *)0xFFFFF000)[0] == page_directory[0]
     * 0xFFC00000 to first_page_table
     * ((uint32_t *)0xFFC00000)[0] == first_page_table[0]
     */
    // And this inside a function
    load_page_directory(page_directory);
    enable_paging();
    return 0;
}

uint32_t *get_physaddr(uint32_t *virtaddr)
{
    uint32_t *addr = 0;
    uint32_t pd_index = (uint32_t)virtaddr >> 22;
    uint32_t pt_index = (uint32_t)virtaddr >> 12 & 0x3ff;
    uint32_t *pd = (uint32_t *)0xFFFFF000;
    uint32_t *pt = (uint32_t *)0xFFC00000;
    if (pd[pd_index] & 0x00000001 && pt[pt_index] & 0x00000001) {
        addr = (uint32_t)pt[pt_index] + ((uint32_t)virtaddr & 0xfff);
        addr = (uint32_t)addr & ~0xfff;
    }
    return addr;
}