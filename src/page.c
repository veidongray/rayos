#include "page.h"
#include "print.h"
#include "systicks.h"
#include "multiboot2.h"

uint32_t page_directory[1024]
    __attribute__((aligned(4096)));
uint32_t first_page_table[1024]
    __attribute__((aligned(4096)));

// 4K page per bit
uint8_t physaddr_bitmap[0x20000];
physaddr_t heap_top = _kernel_end_aligned;

struct page {
    physaddr_t base;
    uint32_t flags;
    uint32_t kref;
} __attribute__((packed));

struct page *page = (void *)0;

int page_init(void)
{
    uint32_t i, klen;

    for (i = 0; i < 0x20000; ++i)
        physaddr_bitmap[i] = 0x00;

    for (i = 0; i < 1024; i++)
    {
        // This sets the following flags to the pages:
        //   Supervisor: Only kernel-mode can access them
        //   Write Enabled: It can be both read from and written to
        //   Not Present: The page table is not present
        page_directory[i] = 0x00000002;
    }
    // we will fill all 1024 entries in the table, mapping 4 megabytes
    klen = (uint32_t)_kernel_end_aligned / 0x1000;
    for (i = 0; i < klen; i++)
    {
        // As the address is page aligned, it will always leave 12 bits zeroed.
        // Those bits are used by the attributes ;)
        first_page_table[i] = (i * 0x1000) | 0x00000003; // attributes: supervisor level, read/write, present.
        set_bitmap(physaddr_bitmap, i);
    }
    // attributes: supervisor level, read/write, present
    page_directory[0] = ((uint32_t)first_page_table) | 0x00000003;
    // 0xC0000000 map to 0x00000000
    page_directory[768] = ((uint32_t)first_page_table) | 0x00000003;
    // self-mapping
    page_directory[1023] = ((uint32_t)page_directory) | 0x00000003;
    /* Map kernel to 0~4MB:
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
    page = (struct page *)alloc_pages((((total_ram >> 12) * sizeof(struct page)) >> 12) + 1);
    for (i = 0; i < ((uint32_t)total_ram >> 12); ++i)
    {
        page[i].base = (physaddr_t)(i * 0x1000);
        page[i].flags = 0;
        if (get_bitmap(physaddr_bitmap, i))
        {
            page[i].flags = 0x1; // used
            page[i].kref = 1;
        }
        else
        {
            page[i].flags = 0;
            page[i].kref = 0;
        }
    }
    cga_info("page[%X].kref = %u, page[%X].base = 0x%X\n",
        (uint32_t)0x100000 >> 12, page[(uint32_t)0x100000 >> 12].kref,
        (uint32_t)0x100000 >> 12, page[(uint32_t)0x100000 >> 12].base);
    map_page((uint32_t *)0x100000, (uint32_t *)0xB0100000, 0x00000003);
    cga_info("page[%X].kref = %u, page[%X].base = 0x%X\n",
        (uint32_t)0x100000 >> 12, page[(uint32_t)0x100000 >> 12].kref,
        (uint32_t)0x100000 >> 12, page[(uint32_t)0x100000 >> 12].base);
    map_page((uint32_t *)0x100000, (uint32_t *)0xB0200000, 0x00000003);
    cga_info("page[%X].kref = %u, page[%X].base = 0x%X\n",
        (uint32_t)0x100000 >> 12, page[(uint32_t)0x100000 >> 12].kref,
        (uint32_t)0x100000 >> 12, page[(uint32_t)0x100000 >> 12].base);
    unmap_page((virtaddr_t)0xB0100000);
    cga_info("page[%X].kref = %u, page[%X].base = 0x%X\n",
        (uint32_t)0x100000 >> 12, page[(uint32_t)0x100000 >> 12].kref,
        (uint32_t)0x100000 >> 12, page[(uint32_t)0x100000 >> 12].base);
    free_page((virtaddr_t)0xB0200000);
    cga_info("page[%X].kref = %u, page[%X].base = 0x%X\n",
        (uint32_t)0x100000 >> 12, page[(uint32_t)0x100000 >> 12].kref,
        (uint32_t)0x100000 >> 12, page[(uint32_t)0x100000 >> 12].base);
    return 0;
}

physaddr_t get_physaddr(virtaddr_t virtaddr)
{
    physaddr_t addr = (physaddr_t)-1;
    uint32_t pd_index = (uint32_t)virtaddr >> 22;
    uint32_t pt_index = (uint32_t)virtaddr >> 12 & 0x3ff;
    pd_t pd = (uint32_t *)0xFFFFF000;
    pt_t pt = (uint32_t *)(0xFFC00000 + (pd_index << 12));
    if ((pd[pd_index] & 0x00000001) && (pt[pt_index] & 0x00000001))
        addr = (physaddr_t)(((uint32_t)pt[pt_index] & ~0xfff) + ((uint32_t)virtaddr & 0xfff));
    return addr;
}

int map_page(uint32_t *physaddr, uint32_t *virtaddr, uint32_t flags)
{
    uint32_t pd_index = (uint32_t)virtaddr >> 22;
    uint32_t pt_index = (uint32_t)virtaddr >> 12 & 0x3ff;
    pd_t pd = (pd_t)0xFFFFF000;
    pt_t pt = (pt_t)(0xFFC00000 + (pd_index << 12));
    pt_t new_pt;

    if (!(pd[pd_index] & 0x00000001))
    {
        new_pt = (pt_t)alloc_page();
        pd[pd_index] = (uint32_t)get_physaddr(new_pt) | 0x00000003;
        new_pt[pt_index] = (uint32_t)(((uint32_t)physaddr & ~0xfff) | flags);
        if (page) page[(uint32_t)physaddr >> 12].kref++;
        flush_tlb();
        return 0;
    }
    if (!(pt[pt_index] & 0x00000001))
    {
        pt[pt_index] = (uint32_t)(((uint32_t)physaddr & ~0xfff) | flags);
        if (page) page[(uint32_t)physaddr >> 12].kref++;
        flush_tlb();
        return 0;
    }
    return -1;
}

int unmap_page(virtaddr_t virtaddr)
{
    uint32_t pd_index = (uint32_t)virtaddr >> 22;
    uint32_t pt_index = (uint32_t)virtaddr >> 12 & 0x3ff;
    pd_t pd = (pd_t)0xFFFFF000;
    pt_t pt = (pt_t)(0xFFC00000 + (pd_index << 12));
    physaddr_t physaddr;
    if ((physaddr = get_physaddr(virtaddr)) == (physaddr_t)-1) return -1;
    if ((pd[pd_index] & 0x00000001) && (pt[pt_index] & 0x00000001))
    {
        pt[pt_index] = 0x00000000;
        if (page) page[(uint32_t)physaddr >> 12].kref--;
        flush_tlb();
        return 0;
    }
    return -1;
}

virtaddr_t alloc_page(void)
{
    uint32_t i, j;
    pd_t pd;
    pt_t pt;
    physaddr_t physaddr;
    virtaddr_t virtaddr;
    uint32_t pd_index;
    uint32_t pt_index;

    physaddr = (physaddr_t)-1;
    virtaddr = (virtaddr_t)-1;

    // find unused physaddr
    for (i = 0; i < 0x100000; ++i)
    {
        if (!get_bitmap(physaddr_bitmap, i))
        {
            physaddr = (physaddr_t)(i * (uint32_t)0x1000);
            // find unused virtaddr
            for (j = 0; j < (uint32_t)0xfffff000; j += (uint32_t)0x1000)
            {
                if (get_physaddr((virtaddr_t)j) == (physaddr_t)-1)
                {
                    // found empty page
                    virtaddr = (virtaddr_t)j;
                    goto found_virtaddr;
                }
            }
        }
    }

found_virtaddr:
    pd_index = (uint32_t)virtaddr >> 22;
    pt_index = (uint32_t)virtaddr >> 12 & 0x3ff;
    pd = (pd_t)0xFFFFF000;
    pt = (pt_t)(0xFFC00000 + (pd_index << 12));
    // check this pde whether need create
    if (!(pd[pd_index] & 0x00000001))
    {
        if (get_physaddr(virtaddr) == (virtaddr_t)-1)
        {
            pd[pd_index] = (uint32_t)((uint32_t)physaddr & ~0xfff) | 0x00000003;
            heap_top = (physaddr_t)((uint32_t)physaddr + 0x1000);
            physaddr = (physaddr_t)((uint32_t)physaddr + 0x1000);
            pt[pt_index] = (uint32_t)((uint32_t)physaddr & ~0xfff) | 0x00000003;
            heap_top = (physaddr_t)((uint32_t)physaddr + 0x1000);
            set_bitmap(physaddr_bitmap, ((uint32_t)physaddr - 0x1000) >> 12);
            set_bitmap(physaddr_bitmap, (uint32_t)physaddr >> 12);
            if (get_physaddr(virtaddr) == (virtaddr_t)-1) return (virtaddr_t)-1;
            if (page) page[(uint32_t)physaddr >> 12].kref++;
            flush_tlb();
            return virtaddr;
        }
    }
    else
    {
        // create a new page
        if (get_physaddr(virtaddr) == (virtaddr_t)-1)
        {
            pt[pt_index] = (uint32_t)((uint32_t)physaddr & ~0xfff) | 0x00000003;
            heap_top = (physaddr_t)((uint32_t)physaddr + 0x1000);
            set_bitmap(physaddr_bitmap, (uint32_t)physaddr >> 12);
            if (get_physaddr(virtaddr) == (virtaddr_t)-1) return (virtaddr_t)-1;
            if (page) page[(uint32_t)physaddr >> 12].kref++;
            flush_tlb();
            return virtaddr;
        }
    }
    return (virtaddr_t)-1;
}

int free_page(virtaddr_t virtaddr)
{
    uint32_t pd_index = (uint32_t)virtaddr >> 22;
    uint32_t pt_index = (uint32_t)virtaddr >> 12 & 0x3ff;
    pd_t pd = (pd_t)0xFFFFF000;
    pt_t pt = (pt_t)(0xFFC00000 + (pd_index << 12));
    if ((pd[pd_index] & 0x00000001) && (pt[pt_index] & 0x00000001))
    {
        uint32_t physaddr = pt[pt_index] & ~0xfff;
        clr_bitmap(physaddr_bitmap, physaddr >> 12);
        if (page) page[(uint32_t)physaddr >> 12].kref--;
        pt[pt_index] = 0x00000000;
        flush_tlb();
        return 0;
    }
    return -1;
}

virtaddr_t alloc_pages(uint32_t num)
{
    uint32_t i, j, len, length;
    virtaddr_t virtaddr[1024];

    for (i = 0; i < num; ++i)
        virtaddr[i] = 0;

    if (num == 0 || num > 1024) return (virtaddr_t)-1;
    if (num == 1) return alloc_page();
    for (i = 0x100000; i < 0xfffff000; i += 0x1000)
    {
        length = 0;
        len = i + (num * 0x1000);
        for (j = i; j < len; j += 0x1000)
            if (get_physaddr((virtaddr_t)j) == (physaddr_t)-1)
                length++;
        if (length == num)
            break;
        i = (j - 0x1000) - (length * 0x1000);
        len = 0xfffff000 - (0x1000 * num);
        if (i > len)
            return (virtaddr_t)-1;
    }
    for (i = 0; i < num; ++i)
    {
        virtaddr[i] = alloc_page();
        if (virtaddr[i] == (virtaddr_t)-1)
        {
            // free already allocated pages
            for (j = 0; j < i; ++j)
                free_page(virtaddr[j]);
            return (virtaddr_t)-1;
        }
    }
    return virtaddr[0];
}

uint8_t get_bitmap(uint8_t *bm, uint32_t index)
{
    uint8_t bit;
    uint32_t i, j;

    i = index >> 3;
    j = index % 8;
    // MSB first
    bit = bm[i] & ((uint32_t)0x80 >> j);
    return (bit) ? 1 : 0;
}

uint8_t set_bitmap(uint8_t *bm, uint32_t index)
{
    uint32_t i, j;

    i = index >> 3;
    j = index % 8;
    // MSB first
    bm[i] = bm[i] | ((1 << 7) >> j);
    return 0;
}

uint8_t clr_bitmap(uint8_t *bm, uint32_t index)
{
    uint32_t i, j;

    i = index >> 3;
    j = index % 8;
    // MSB first
    bm[i] = bm[i] & ~((1 << 7) >> j);
    return 0;
}

int flush_tlb(void)
{
    asm volatile("movl %cr3, %eax\r\n"
                 "movl %eax, %cr3\r\n");
    return 0;
}