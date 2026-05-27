#include <page.h>
#include <bitmap.h>
#include <alignes.h>
#include <multiboot2.h>

#define BOOTMAP_LEN 0x1000000 // 128MB

bitmap_t page_alloc_bitmap;
static uint64_t *page_bitmap_data;
extern uint64_t _kernel_phys_end_aligned[];
extern uint64_t _kernel_virt_end_aligned[];

void page_init(void)
{
    size_t mem;
    size_t bits;
    size_t bitmap_data_bytes;

    mem = ALIGNED_UP(get_total_mem(), PAGE_SIZE);
    bits = mem >> PAGE_SHIFT;
    bitmap_data_bytes = ALIGNED_UP(bits >> 3, PAGE_SIZE);
    page_bitmap_data = (uint64_t *)_kernel_virt_end_aligned;

    bitmap_init(&page_alloc_bitmap, page_bitmap_data, bits);
    // mark used page
    bitmap_set_range(&page_alloc_bitmap, 0, ((uint64_t)_kernel_phys_end_aligned + bitmap_data_bytes) >> PAGE_SHIFT);
    // unmap unused page
    unmap_page_range((uint64_t)_kernel_virt_end_aligned + bitmap_data_bytes,
                     (BOOTMAP_LEN - (uint64_t)_kernel_phys_end_aligned - bitmap_data_bytes) >> PAGE_SHIFT);
}

uint64_t alloc_pages(size_t order)
{
    int ret;
    int start;

    start = bitmap_find_first_zero(&page_alloc_bitmap, 0);
    ret = bitmap_alloc_range(&page_alloc_bitmap, order_to_pages(order), start);
    if (ret < 0)
    {
        return ret;
    }
    return ret * PAGE_SIZE;
}

void free_pages(uint64_t physaddr, size_t order)
{
    bitmap_free_range(&page_alloc_bitmap, physaddr >> PAGE_SHIFT, order_to_pages(order));
}

int map_page_range(uint64_t physaddr, uint64_t virtaddr, uint64_t flags, size_t len)
{
    size_t i, j;
    uint64_t va, pa;
    uint64_t pml4_idx, pdpt_idx, pd_idx, pt_idx;
    uint64_t *map_pml4, *map_pdpt, *map_pd, *map_pt;

    va = virtaddr;
    pa = physaddr;

    for (i = 0; i < len; i++, pa += 0x1000, va += 0x1000)
    {
        pml4_idx = (va >> 39) & 0x1FF;
        pdpt_idx = (va >> 30) & 0x1FF;
        pd_idx = (va >> 21) & 0x1FF;
        pt_idx = (va >> 12) & 0x1FF;

        // 所有地址均为 canonical（高16位全1），适用于 x86-64 递归页表映射
        map_pml4 = (uint64_t *)(PML4_BASE << 12ULL);
        map_pdpt = (uint64_t *)((PML4_BASE << 21ULL) + (pml4_idx << 12ULL));
        map_pd = (uint64_t *)((PML4_BASE << 30ULL) + (pml4_idx << 21ULL) + (pdpt_idx << 12ULL));
        map_pt = (uint64_t *)((PML4_BASE << 39ULL) + (pml4_idx << 30ULL) + (pdpt_idx << 21ULL) + (pd_idx << 12ULL));

        if (!(map_pml4[pml4_idx] & 0x1ULL))
        {
            map_pml4[pml4_idx] = alloc_page() | flags | 0x1ULL;
            for (j = 0; j < 512; j++)
            {
                // clear new pdpt page
                map_pdpt[j] = 0x0000000000000000;
            }
        }
        if (!(map_pdpt[pdpt_idx] & 0x1ULL))
        {
            map_pdpt[pdpt_idx] = alloc_page() | flags | 0x1ULL;
            for (j = 0; j < 512; j++)
            {
                // clear new pd page
                map_pd[j] = 0x0000000000000000;
            }
        }
        if (!(map_pd[pd_idx] & 0x1ULL))
        {
            map_pd[pd_idx] = alloc_page() | flags | 0x1ULL;
            for (j = 0; j < 512; j++)
            {
                // clear new pt page
                map_pt[j] = 0x0000000000000000;
            }
        }
        if (map_pt[pt_idx] & 0x1ULL)
        {
            // already map
            unmap_page_range(virtaddr, i);
            return i;
        }
        map_pt[pt_idx] = (pa & ~0xfff) | flags | 0x1ULL;
        asm volatile(
            "movq %cr3, %rax\r\n"
            "movq %rax, %cr3\r\n");
    }
    return i;
}

int unmap_page_range(uint64_t virtaddr, size_t len)
{
    size_t i;
    uint64_t va;
    uint64_t pml4_idx, pdpt_idx, pd_idx, pt_idx;
    uint64_t *map_pml4, *map_pdpt, *map_pd, *map_pt;

    va = virtaddr;

    for (i = 0; i < len; i++, va += 0x1000)
    {
        pml4_idx = (va >> 39) & 0x1FF;
        pdpt_idx = (va >> 30) & 0x1FF;
        pd_idx = (va >> 21) & 0x1FF;
        pt_idx = (va >> 12) & 0x1FF;

        map_pml4 = (uint64_t *)(PML4_BASE << 12ULL);
        if (!(map_pml4[pml4_idx] & 0x1ULL))
        {
            return i;
        }
        map_pdpt = (uint64_t *)((PML4_BASE << 21ULL) + (pml4_idx << 12ULL));
        if (!(map_pdpt[pdpt_idx] & 0x1ULL))
        {
            return i;
        }
        map_pd = (uint64_t *)((PML4_BASE << 30ULL) + (pml4_idx << 21ULL) + (pdpt_idx << 12ULL));
        if (!(map_pd[pd_idx] & 0x1ULL))
        {
            return i;
        }
        map_pt = (uint64_t *)((PML4_BASE << 39ULL) + (pml4_idx << 30ULL) + (pdpt_idx << 21ULL) + (pd_idx << 12ULL));
        if (map_pt[pt_idx] & 0x1ULL)
        {
            // already map
            map_pt[pt_idx] = 0x0000000000000000ULL;
            asm volatile(
                "movq %cr3, %rax\r\n"
                "movq %rax, %cr3\r\n");
        }
        else
        {
            return i;
        }
    }
    return i;
}

uint64_t get_physaddr(uint64_t virtaddr)
{
    uint64_t pml4_idx, pdpt_idx, pd_idx, pt_idx;
    uint64_t *map_pml4, *map_pdpt, *map_pd, *map_pt;

    pml4_idx = (virtaddr >> 39) & 0x1FF;
    pdpt_idx = (virtaddr >> 30) & 0x1FF;
    pd_idx = (virtaddr >> 21) & 0x1FF;
    pt_idx = (virtaddr >> 12) & 0x1FF;

    map_pml4 = (uint64_t *)(PML4_BASE << 12ULL);
    if (!(map_pml4[pml4_idx] & 0x1ULL))
    {
        return -1;
    }
    map_pdpt = (uint64_t *)((PML4_BASE << 21ULL) + (pml4_idx << 12ULL));
    if (!(map_pdpt[pdpt_idx] & 0x1ULL))
    {
        return -1;
    }
    map_pd = (uint64_t *)((PML4_BASE << 30ULL) + (pml4_idx << 21ULL) + (pdpt_idx << 12ULL));
    if (!(map_pd[pd_idx] & 0x1ULL))
    {
        return -1;
    }
    map_pt = (uint64_t *)((PML4_BASE << 39ULL) + (pml4_idx << 30ULL) + (pdpt_idx << 21ULL) + (pd_idx << 12ULL));
    if (map_pt[pt_idx] & 0x1ULL)
    {
        // already map
        return (map_pt[pt_idx] & ~0xfff) + (virtaddr & 0xfff);
    }
    return -1;
}

void load_pml4(uint64_t pml4_physaddr)
{
    asm volatile(
        "movq %0, %%rax\r\n"
        "movq %%rax, %%cr3\r\n"
        :
        : "r"(pml4_physaddr)
        : "rax");
}

uint64_t order_to_pages(size_t order)
{
    return 0x1 << order;
}

uint64_t size_to_order(size_t size)
{
    if (size == 0)
        return 0;
    size_t pages = (size + (1UL << PAGE_SHIFT) - 1) >> PAGE_SHIFT;
    return fls(pages - 1);
}