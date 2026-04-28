#include <stdint.h>
#include "multiboot2.h"
#include "paging.h"
#include "print.h"
#include "kheap.h"
#include "libc/stdlib.h"

extern uint32_t _virt_offset[];
extern uint32_t _kernel_base[];
extern uint32_t _kernel_virt_start[];
extern uint32_t _kernel_phys_start[];
extern uint32_t _kernel_virt_end[];
extern uint32_t _kernel_phys_end[];
extern uint32_t _kernel_virt_end_aligned[];
extern uint32_t _kernel_phys_end_aligned[];
extern uint32_t host_total_mem;

uint32_t kpage_directory[1024] __attribute__((aligned(4096)));
static uint32_t *kpage_tables;

#define PAGE_USED (1UL << 0)
#define PAGE_UNUSED (0xfffffffeUL)

// page空闲列表
struct page *page_list;
uint32_t kheap_top = 0;

int page_init(void)
{
    uint32_t i;

    // Calculate the total number of pages needed for the host memory
    // 计算所有可用内存的页数和页表数
    uint32_t ktotal_pages =
        (host_total_mem / 0x1000) + ((host_total_mem % 0x1000) ? 1 : 0); // Round up to nearest page
    uint32_t ktotal_page_tables =
        (ktotal_pages / 0x400) + ((ktotal_pages % 0x400) ? 1 : 0); // Each page table can map 1024 pages

    // 计算内核大小从0xc0000000开始
    // 并且加上尾部的page tables大小
    uint32_t kernel_size =
        ((uint32_t)_kernel_virt_end_aligned - (uint32_t)_virt_offset) + ((uint32_t)ktotal_page_tables * 0x1000);
    kernel_size += ((kernel_size % 4096) != 0) ? (4096 - (kernel_size % 4096)) : 0; // 对齐4K

    // Map all memory up to the total memory size, including the kernel itself
    // 映射所有的系统可用内存空间
    // 从0xC0000000映射到0x00000000开始
    kpage_tables = (uint32_t *)((uint32_t)_kernel_virt_end_aligned);
    for (i = 0; i < ktotal_pages; i++)
        kpage_tables[i] = (i * 0x1000) | 0x3; // Present + Read/Write
    for (i = 0; i < ktotal_page_tables; i++)
        kpage_directory[i + 0x300] =
            (uint32_t)((uint32_t)kpage_tables - (uint32_t)_virt_offset + (i * 0x1000)) | 0x3;     // Present + Read/Write
    kpage_directory[1023] = (uint32_t)((uint32_t)kpage_directory - (uint32_t)_virt_offset) | 0x3; // Map the last page to itself for recursive paging
    load_page_directory((uint32_t *)((uint32_t)kpage_directory - (uint32_t)_virt_offset));
    enable_paging();

    // 设置page列表，使用全局数组记录每个page
    // 目的是方便后续alloc_page分配页以及page的管理
    kheap_top = (uint32_t)_virt_offset + kernel_size;
    page_list = (struct page *)kheap_top;
    for (i = 0; i < ktotal_pages; i++)
    {
        page_list[i].base = (uint32_t *)(i * 0x1000);
        page_list[i].flags = PAGE_UNUSED;
        page_list[i].kref = 0;
    }

    // kheap_top增加page列表的空间
    // kheap_top向上(高地址)增长
    kheap_top += (sizeof(struct page) * ktotal_pages);
    kheap_top += (((sizeof(struct page) * ktotal_pages) % 4096) != 0) ? (4096 - ((sizeof(struct page) * ktotal_pages) % 4096)) : 0; // 对齐4K
    // 将内核已覆盖的内存页的标志改为已使用
    for (i = 0; i < (kheap_top + KHEAP_SIZE - (uint32_t)_virt_offset) / 0x1000; i++)
    {
        page_list[i].flags &= PAGE_USED;
        page_list[i].kref = 1;
    }
    return 0;
}

struct page *alloc_page(void)
{
    struct page *fp = NULL;
    // 遍历page列表寻找空闲页
    for (fp = page_list; (uint32_t)fp->base < (uint32_t)_virt_offset + host_total_mem; fp++)
    {
        if (fp->flags == PAGE_UNUSED)
        {
            fp->flags = PAGE_USED;
            fp->kref = 0;
            return fp;
        }
    }
    return NULL;
}

void free_page(struct page *page)
{
    if (page->kref <= 0)
    {
        page->flags = PAGE_UNUSED;
        page->kref = 0;
    }
    else
        page->kref--;
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

void map_page(void *physaddr, void *virtualaddr, unsigned int flags)
{
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
        struct page *t;
        t = alloc_page();
        t->kref = 1;
        pd[pdindex] = (uint32_t)t->base | 0x3;
    }

    unsigned long *pt = ((unsigned long *)0xFFC00000UL) + (0x400UL * pdindex);
    // Here you need to check whether the PT entry is present.
    // When it is, then there is already a mapping present. What do you do now?
    // 如果虚拟地址已经有映射则直接返回
    if ((pt[ptindex] & 0x1UL))
        return;

    physaddr = (void *)((uint32_t)physaddr & 0xfffff000UL);
    page_list[(uint32_t)physaddr / 4096].flags = PAGE_USED;
    page_list[(uint32_t)physaddr / 4096].kref++;
    pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFFUL) | 0x01UL; // Present

    // Now you need to flush the entry in the TLB
    // or you might not notice the change.
    flush_tlb();
}

void flush_tlb(void)
{
    asm volatile(
        "mov %cr3, %eax\r\n"
        "mov %eax, %cr3\r\n");
}