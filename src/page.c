#include <stdint.h>
#include "page.h"
#include "bitmap.h"
#include "multiboot2.h"

#define PAGE_SIZE 0x1000ULL
#define KERNEL_BASE 0xFFFFFFFFC0000000ULL
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

        pml4[511] = ((uint64_t)pdpt - KERNEL_BASE) | 0x3ULL;
        pdpt[511] = ((uint64_t)pd - KERNEL_BASE) | 0x3ULL;
        pt = kernel_virt_end_aligned;
        for (i = 0; i < total_pde; i++)
        {
            pd[i] = ((uint64_t)pt + (i * PAGE_SIZE) - KERNEL_BASE) | 0x3ULL;
        }
        pd[511] = ((uint64_t)(&(pt[511])) - KERNEL_BASE) | 0x3ULL;

        for (i = 0; i < total_pte; i++)
        {
            pt[i] = (i * PAGE_SIZE) | 0x3ULL;
            kernel_virt_end_aligned++;
        }
        pt[511] = ((uint64_t)pml4 - KERNEL_BASE) | 0x3ULL;

        // mark used page
        bitmap_init(&bmp_page_map, page_map, 512 * 64);
        for (i = 0; i < (uint64_t)kernel_virt_end_aligned - KERNEL_BASE; i += PAGE_SIZE)
        {
            bitmap_set(&bmp_page_map, i >> 12);
        }
    }

    load_pml4((uint64_t)pml4 - KERNEL_BASE);

    *(volatile uint8_t *)(KERNEL_BASE + 0xb8000) = '?';
    asm volatile(
        "movq %0, %%rax\r\n"
        "movq %1, %%rbx\r\n"
        "movq %2, %%rcx\r\n"
        "movq %3, %%rdx\r\n"
        "movq %4, %%r8\r\n"
        :
        : "r"(get_cr3()), "r"(pdpt), "r"(pd), "r"(pt), "r"(kernel_virt_end_aligned)
        : "rax", "rbx", "rcx", "rdx", "r8");
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
    uint64_t pml4_index;
    uint64_t pdpt_index;
    uint64_t pd_index;
    uint64_t pt_index;
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;

    pml4_index = (physaddr >> 39) & 0x1ffULL;
    pdpt_index = (physaddr >> 30) & 0x1ffULL;
    pd_index = (physaddr >> 21) & 0x1ffULL;
    pt_index = (physaddr >> 12) & 0x1ffULL;
    pml4 = get_cr3();
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