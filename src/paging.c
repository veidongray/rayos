#include <stdint.h>
#include "multiboot2.h"
#include "paging.h"
#include "print.h"
#include "libc/stdlib.h"
#include "mm.h"
#include "panic.h"
#include "idt.h"

extern uint32_t _virt_offset[];
extern uint32_t _kernel_phys_end_aligned[];
static uint32_t total_pages;
static uint32_t total_tables;
static uint32_t early_tables[4096] __attribute__((aligned(4096)));
static uint32_t kpage_directory[1024] __attribute__((aligned(4096)));
static uint32_t *kpage_tables = NULL;
static struct page *pages = NULL;

void early_page_init(void)
{
    uint32_t i;
    uint32_t cr3, *cr3ptr;
    get_cr3(&cr3);

    for (i = 0; i < 4096; i++)
    {
        early_tables[i] = ((i * 0x1000UL)) | 0x3UL;
    }

    // early map 16MB
    cr3ptr = (uint32_t *)(cr3 + (uint32_t)_virt_offset);
    cr3ptr[768] = ((uint32_t)(early_tables + 0) - (uint32_t)_virt_offset) | 0x3UL;
    cr3ptr[769] = ((uint32_t)(early_tables + 1024) - (uint32_t)_virt_offset) | 0x3UL;
    cr3ptr[770] = ((uint32_t)(early_tables + 2048) - (uint32_t)_virt_offset) | 0x3UL;
    cr3ptr[771] = ((uint32_t)(early_tables + 3072) - (uint32_t)_virt_offset) | 0x3UL;
}

int page_init(void)
{
    uint32_t i;
    struct page *t;

    // Calculate the total number of pages needed for the host memory
    // 计算所有可用内存的页数和页表数
    total_pages = host_total_mem / 4096;
    total_tables = total_pages / 1024;

    // Map all memory up to the total memory size, including the kernel itself
    // 映射所有的系统可用内存空间
    // 从0xC0000000映射到0x00000000开始
    kpage_tables = (uint32_t *)early_malloc(total_pages * sizeof(uint32_t));
    for (i = 0; i < total_pages; i++)
    {
        kpage_tables[i] = (i * 0x1000UL) | 0x3UL;
    }
    for (i = 0; i < total_tables; i++)
    {
        kpage_directory[i + 768] = ((uint32_t)kpage_tables - (uint32_t)_virt_offset + (i * 0x1000UL)) | 0x3UL;
    }
    kpage_directory[1023] = ((uint32_t)kpage_directory - (uint32_t)_virt_offset) | 0x3UL;
    load_page_directory((uint32_t *)((uint32_t)kpage_directory - (uint32_t)_virt_offset));
    enable_paging();

    pages = (struct page *)early_malloc(sizeof(struct page) * total_pages);
    for (i = 0; i < total_pages; i++)
    {
        pages[i].base = (uint32_t *)(i * 0x1000);
        pages[i].kref = 0;
    }
    for (t = pages; (uint32_t)t->base < (uint32_t)_kernel_phys_end_aligned; t++)
    {
        t->kref++;
    }
    return 0;
}

struct page *alloc_page(void)
{
    uint32_t i;
    struct page *fp = NULL;
    // 遍历page列表寻找空闲页
    disable_irq();
    for (fp = pages, i = 0; i < total_pages; fp++)
    {
        if (!fp->kref)
        {
            fp->kref++;
            goto done;
        }
    }

done:
    enable_irq();
    return fp;
}

void free_page(struct page *ptr)
{
    disable_irq();
    ptr->kref = 0;
    enable_irq();
}

void *get_physaddr(void *virtualaddr)
{
    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FFUL;

    unsigned long *pd = (unsigned long *)0xFFFFF000UL;
    // Here you need to check whether the PD entry is present.
    if ((pd[pdindex] & 0x1) == 0)
        return NULL;

    unsigned long *pt = ((unsigned long *)0xFFC00000UL) + (0x400UL * pdindex);
    // Here you need to check whether the PT entry is present.
    if ((pt[ptindex] & 0x1) == 0)
        return NULL;

    return (void *)((pt[ptindex] & ~0xFFFUL) + ((unsigned long)virtualaddr & 0xFFFUL));
}

int map_page(void *physaddr, void *virtualaddr, unsigned int flags)
{
    struct page *t;

    // Make sure that both addresses are page-aligned.
    unsigned long pdindex = (unsigned long)virtualaddr >> 22UL;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12UL & 0x03FFUL;

    unsigned long *pd = (unsigned long *)0xFFFFF000UL;
    // Here you need to check whether the PD entry is present.
    // When it is not present, you need to create a new empty PT and
    // adjust the PDE accordingly.
    // 如果对应页表不存在则分配一个页作为页表空间
    if ((pd[pdindex] & 0x1) == 0)
    {
        t = alloc_page();
        pd[pdindex] = (uint32_t)t->base | (flags & 0xFFFUL) | 0x01UL;
    }

    unsigned long *pt = ((unsigned long *)0xFFC00000UL) + (0x400UL * pdindex);
    // Here you need to check whether the PT entry is present.
    // When it is, then there is already a mapping present. What do you do now?
    // 如果虚拟地址已经有映射则直接返回
    if ((pt[ptindex] & 0x1UL))
        return -1;

    physaddr = (void *)((uint32_t)physaddr & 0xfffff000UL);
    pages[(uint32_t)physaddr / 4096].kref++;
    pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFFUL) | 0x01UL; // Present

    // Now you need to flush the entry in the TLB
    // or you might not notice the change.
    flush_tlb();
    return 0;
}

void flush_tlb(void)
{
    asm volatile(
        "mov %cr3, %eax\r\n"
        "mov %eax, %cr3\r\n");
}

void get_cr3(uint32_t *cr3)
{
    asm volatile(
        "movl %%cr3, %0"
        : "=r"(*cr3)
        :
        : "memory");
}