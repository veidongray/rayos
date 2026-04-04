#include "page.h"
#include "print.h"

static pd_t kernel_page_directory_entry[1024] __attribute__((aligned(4096)));
static pt_t kernel_page_table_entry[8][1024] __attribute__((aligned(4096)));
struct vm_area
{
    pd_t pd[1024];
    pd_t pt[1024][1024];
};

int page_init(void)
{
    uint32_t i, j, len, size;

    for (i = 0; i < 1024; ++i)
    {
        // fill global page directory
        kernel_page_directory_entry[i] = 0x00000000;
    }

    /* Init kernel page from page[0 ~ len] and page[768 ~ 768 + len] */
    size = (uint32_t)_kernel_end_aligned - (uint32_t)_kernel_start;
    len = (size / 0x400000) + ((size % 0x400000) ? 1 : 0);

    for (i = 0; i < len; ++i)
    {
        for (j = 0; j < 1024; ++j)
        {
            // 4MB per page == 0x400000 bytes
            kernel_page_table_entry[i][j] = ((i * 0x400000) + (j * 0x1000));
            page_set_rw(&kernel_page_table_entry[i][j]);
            page_set_present(&kernel_page_table_entry[i][j]);
        }
    }

    for (i = 0; i < len; ++i)
    {
        // for 0x00000000 ~ [size]
        kernel_page_directory_entry[i] = (uint32_t)kernel_page_table_entry[i];
        page_set_rw(&kernel_page_directory_entry[i]);
        page_set_present(&kernel_page_directory_entry[i]);
    }

    for (i = 0; i < len; ++i)
    {
        // for 0xC0000000 ~ 0xFFFFFFFF
        kernel_page_directory_entry[i + 768] = (uint32_t)kernel_page_table_entry[i];
        page_set_rw(&kernel_page_directory_entry[i]);
        page_set_present(&kernel_page_directory_entry[i]);
    }

    // last PDE to itslef
    kernel_page_directory_entry[1023] = kernel_page_directory_entry;
    page_set_rw(&kernel_page_directory_entry[1023]);
    page_set_present(&kernel_page_directory_entry[1023]);

    // And this inside a function
    load_page_directory(kernel_page_directory_entry);
    enable_paging();

    cga_printf("Pageing:\n");
    for (i = 768; i < len + 768; i += 4)
    {
        cga_printf("PDE[%u]: 0x%X PDE[%u]: 0x%X PDE[%u]: 0x%X PDE[%u]: 0x%X\n",
                   i, kernel_page_directory_entry[i],
                   i + 1, kernel_page_directory_entry[i + 1],
                   i + 2, kernel_page_directory_entry[i + 2],
                   i + 3, kernel_page_directory_entry[i + 3]);
    }
    cga_printf("last PDE[1023]: 0x%X\n", *(uint32_t *)0xfffff000);
    return 0;
}

int page_set_rw(uint32_t *page)
{
    *page = *page | 0x00000002;
    return *page;
}

int page_set_present(uint32_t *page)
{
    *page = *page | 0x00000001;
    return *page;
}

uint32_t page_get_physaddr(uint32_t virtualaddr)
{
    uint32_t addr = 0;
    uint32_t pd_index;
    uint32_t pt_index;
    pd_t *last_pde = (pd_t *)0xfffff000;
    pt_t *pte;

    pd_index = virtualaddr >> 22;
    pt_index = (virtualaddr >> 12) & 0x3ff;

    // check last pde[pd_index] whether set PRESENT bit
    if (last_pde[pd_index] && 0x00000001)
    {
        pte = last_pde[pd_index] & 0xfffffffc;
    }
    else
    {
        goto err;
    }

    // check the pte whether set PRESENT bit
    if (*(uint32_t *)pte && 0x00000001)
    {
        addr = *(uint32_t *)pte & 0xfffffffc;
    }
    else
    {
        goto err;
    }

    // add page offsets
    addr += (pt_index * 0x1000) + (virtualaddr & 0xfff);
    return addr;
err:
    cga_printf("Error virtual address 0x%X\n", virtualaddr);
    return 0;
}