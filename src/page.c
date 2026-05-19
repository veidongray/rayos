#include <stdint.h>
#include "page.h"
#include "bitmap.h"
#include "multiboot2.h"

#define PAGE_SIZE 0x1000ULL
#define KERNEL_BASE 0xffff800000000000ULL
#define PML4_BASE 0xFFFFFFFFFFFFFULL
#define ALIGN_4K(val) (((uint64_t)(val) & ~0xfff) + 4096)

__attribute__((aligned(4096))) static uint64_t pml4[512];
__attribute__((aligned(4096))) static uint64_t pdpt[512];
__attribute__((aligned(4096))) static uint64_t pd[512];
static uint64_t *pt;
static uint64_t *kernel_virt_end_aligned;
extern uint64_t _kernel_virt_end_aligned[];
static uint64_t page_map[512];
static bitmap_t bmp_page_map;

uint64_t alloc_page(void);
uint64_t get_cr3(void);

void page_init(void)
{
    uint64_t i;
    uint64_t total_pte;
    uint64_t total_pde;

    kernel_virt_end_aligned = (uint64_t *)_kernel_virt_end_aligned;
    if (get_total_mem() <= 128 * 1024 * 1024)
    {
        total_pte = (128 * 1024 * 1024) / PAGE_SIZE;
        total_pde = (total_pte / 512);

        pml4[511] = ((uint64_t)pml4 - KERNEL_BASE) | 0x3ULL;
        pml4[256] = ((uint64_t)pdpt - KERNEL_BASE) | 0x3ULL;
        pdpt[0] = ((uint64_t)pd - KERNEL_BASE) | 0x3ULL;

        pt = kernel_virt_end_aligned;
        for (i = 0; i < total_pde; i++)
        {
            pd[i] = ((uint64_t)pt + (i * PAGE_SIZE) - KERNEL_BASE) | 0x3ULL;
        }

        for (i = 0; i < total_pte; i++)
        {
            pt[i] = (i * PAGE_SIZE) | 0x3ULL;
            kernel_virt_end_aligned++;
        }

        // mark used page
        bitmap_init(&bmp_page_map, page_map, 512 * 64);
        for (i = 0; i < (uint64_t)kernel_virt_end_aligned - KERNEL_BASE; i += PAGE_SIZE)
        {
            bitmap_set(&bmp_page_map, i >> 12);
        }
    }

    load_pml4((uint64_t)pml4 - KERNEL_BASE);

    *(volatile uint8_t *)(KERNEL_BASE + 0xb8000) = '?';
    map_page(0xb8000, 0xffff7000000b8000ULL, 0x3);
    *(volatile uint8_t *)0xffff7000000b8000ULL = '&';
}

uint64_t alloc_page(void)
{
    int ret;
    ret = bitmap_find_first_zero(&bmp_page_map, 0);
    if (ret >= 0)
    {
        bitmap_set(&bmp_page_map, ret);
        return ret * PAGE_SIZE;
    }
    return ret;
}

void free_page(uint64_t physaddr)
{
    bitmap_clear(&bmp_page_map, physaddr >> 12);
}

int map_page(uint64_t physaddr, uint64_t virtaddr, uint64_t flags)
{
    uint64_t pml4_idx, pdpt_idx, pd_idx, pt_idx;
    uint64_t *map_pml4, *map_pdpt, *map_pd, *map_pt;

    pml4_idx = (virtaddr >> 39) & 0x1FF;
    pdpt_idx = (virtaddr >> 30) & 0x1FF;
    pd_idx = (virtaddr >> 21) & 0x1FF;
    pt_idx = (virtaddr >> 12) & 0x1FF;

    // 所有地址均为 canonical（高16位全1），适用于 x86-64 递归页表映射
    map_pml4 = (uint64_t *)(PML4_BASE << 12ULL);
    if (!(map_pml4[pml4_idx] & 0x1ULL))
    {
        map_pml4[pml4_idx] = alloc_page() | flags | 0x1ULL;
    }
    map_pdpt = (uint64_t *)((PML4_BASE << 21ULL) + (pml4_idx << 12ULL));
    if (!(map_pdpt[pdpt_idx] & 0x1ULL))
    {
        map_pdpt[pdpt_idx] = alloc_page() | flags | 0x1ULL;
    }
    map_pd = (uint64_t *)((PML4_BASE << 30ULL) + (pml4_idx << 21ULL) + (pdpt_idx << 12ULL));
    if (!(map_pd[pd_idx] & 0x1ULL))
    {
        map_pd[pd_idx] = alloc_page() | flags | 0x1ULL;
    }
    map_pt = (uint64_t *)((PML4_BASE << 39ULL) + (pml4_idx << 30ULL) + (pdpt_idx << 21ULL) + (pd_idx << 12ULL));
    if (map_pt[pt_idx] & 0x1ULL)
    {
        // already map
        return -1;
    }
    map_pt[pt_idx] = (physaddr & ~0xfff) | flags | 0x1ULL;
    asm volatile(
        "movq %cr3, %rax\r\n"
        "movq %rax, %cr3\r\n");
}

uint64_t get_cr3(void)
{
    uint64_t retval;

    asm volatile(
        "movq %%cr3, %0"
        : "=r"(retval)
        :
        : "rax");
    return retval;
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