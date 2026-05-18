#include <stdint.h>
#include "page.h"
#include "multiboot2.h"

#define KERNEL_BASE 0xFFFFFFFFC0000000ULL
#define ALIGN_4K(val) (((uint64_t)(val) & ~0xfff) + 4096)

__attribute__((aligned(4096))) static uint64_t pml4[512];
__attribute__((aligned(4096))) static uint64_t pdpt[512];
__attribute__((aligned(4096))) static uint64_t pd[512];
static uint64_t *pt;
static uint64_t *kernel_virt_end_aligned;
extern uint64_t _kernel_virt_end_aligned[];

void page_init(void)
{
    uint64_t i;
    uint64_t total_pte;
    uint64_t total_pde;

    kernel_virt_end_aligned = (uint64_t *)_kernel_virt_end_aligned;
    if (get_total_mem() <= 128 * 1024 * 1024)
    {
        total_pte = (128 * 1024 * 1024) / 4096;
        total_pde = (total_pte / 512);

        pml4[511] = ((uint64_t)pdpt - KERNEL_BASE) | 0x3ULL;
        pdpt[511] = ((uint64_t)pd - KERNEL_BASE) | 0x3ULL;
        pt = kernel_virt_end_aligned;
        for (i = 0; i < total_pde; i++)
        {
            pd[i] = ((uint64_t)pt + (i * 4096) - KERNEL_BASE) | 0x3ULL;
        }
        for (i = 0; i < total_pte; i++)
        {
            pt[i] = (i * 0x1000ULL) | 0x3ULL;
            kernel_virt_end_aligned++;
        }
    }

    load_pml4((uint64_t)pml4 - KERNEL_BASE);
    asm volatile(
        "movq %0, %%rax\r\n"
        "movq %1, %%rbx\r\n"
        "movq %2, %%rcx\r\n"
        "movq %3, %%rdx\r\n"
        "movq %4, %%r8\r\n"
        :
        : "r"(pml4), "r"(pdpt), "r"(pd), "r"(pt), "r"(kernel_virt_end_aligned)
        : "rax", "rbx", "rcx", "rdx", "r8");
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