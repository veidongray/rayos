#include <stdint.h>
#include "multiboot2.h"

#define ALIGN_4K(val) (((uint64_t)(val) & ~0xfff) + 4096)

static uint64_t *pml4;
static uint64_t *pdpt;
static uint64_t *pd;
static uint64_t *pt;
static uint64_t *kernel_virt_end_aligned;
extern uint64_t _kernel_virt_end_aligned[];

void page_init(void)
{
    uint64_t total_pte;
    uint64_t total_pde;
    uint64_t total_pdpte;
    uint64_t total_pml4e;

    kernel_virt_end_aligned = (uint64_t *)_kernel_virt_end_aligned;
    total_pte = get_total_mem() / 4096;
    total_pde = (total_pte / 512) + 1;
    total_pdpte = (total_pde / 512) + 1;
    total_pml4e = (total_pdpte / 512) + 1;

    pml4 = kernel_virt_end_aligned;
    kernel_virt_end_aligned = kernel_virt_end_aligned + 512;

    pdpt = kernel_virt_end_aligned;
    if (total_pdpte <= 512)
    {
        kernel_virt_end_aligned = kernel_virt_end_aligned + 512;
    }
    else
    {
        kernel_virt_end_aligned = (uint64_t *)ALIGN_4K(kernel_virt_end_aligned + total_pdpte);
    }

    pd = kernel_virt_end_aligned;
    if (total_pde <= 512)
    {
        kernel_virt_end_aligned = kernel_virt_end_aligned + 512;
    }
    else
    {
        kernel_virt_end_aligned = (uint64_t *)ALIGN_4K(kernel_virt_end_aligned + total_pde);
    }

    pt = kernel_virt_end_aligned;
    kernel_virt_end_aligned = (uint64_t *)ALIGN_4K(kernel_virt_end_aligned + total_pte);
    asm volatile(
        "movq %0, %%rax\r\n"
        "movq %1, %%rbx\r\n"
        "movq %2, %%rcx\r\n"
        "movq %3, %%rdx\r\n"
        "movq %4, %%r8\r\n"
        "hlt\r\n"
        :
        : "r"(pml4), "r"(pdpt), "r"(pd), "r"(pt), "r"(kernel_virt_end_aligned)
        : "rax", "rbx", "rcx", "rdx", "r8");
}