#include <stdint.h>
#include "multiboot2.h"
#include "paging.h"
#include "print.h"
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

extern void load_page_directory(uint32_t *page_directory);
extern void enable_paging();

static uint32_t kpage_directory[1024] __attribute__((aligned(4096)));
static uint32_t *kpage_tables;

struct page {
    uint32_t *base;
    int32_t kref;
    uint32_t flags;
} __attribute__((packed));

#define PAGE_USED (1UL << 0)
#define PAGE_UNUSED (0xfffffffeUL)

struct page *page_list;
uint32_t *kheap_pool_start = 0;
uint32_t kheap_top = 0;
struct page *alloc_page(void);
void free_page(struct page *page);

int page_init(void)
{
    uint32_t i;
    // 还没有实现好kmalloc的时候使用 kheap_top 来标定分配空间的起始地址
    kheap_top = (uint32_t)_kernel_virt_end_aligned;

    // Calculate the total number of pages needed for the host memory
    uint32_t ktotal_pages =
        (host_total_mem / 0x1000) + ((host_total_mem % 0x1000) ? 1 : 0); // Round up to nearest page
    uint32_t ktotal_page_tables =
        (ktotal_pages / 0x400) + ((ktotal_pages % 0x400) ? 1 : 0); // Each page table can map 1024 pages

    uint32_t kernel_size =
        (uint32_t)kheap_top - (uint32_t)_kernel_virt_start + (uint32_t)0x1000 + (uint32_t)ktotal_page_tables * 0x1000;
    kpage_tables = (uint32_t *)((uint32_t)kheap_top); // Place page tables right after the kernel
    kheap_top = (uint32_t *)(kernel_size + (uint32_t)_kernel_base); // Start the heap right after the kernel and page tables

    // Map all memory up to the total memory size, including the kernel itself
    // 映射所有的系统可用内存空间
    for (i = 0; i < ktotal_pages; i++)
        kpage_tables[i] = (i * 0x1000) | 0x3; // Present + Read/Write
    for (i = 0; i < ktotal_page_tables; i++)
        kpage_directory[i + 0x300] =
            (uint32_t)((uint32_t)kpage_tables - (uint32_t)_virt_offset + (i * 0x1000)) | 0x3;     // Present + Read/Write
    kpage_directory[1023] = (uint32_t)((uint32_t)kpage_directory - (uint32_t)_virt_offset) | 0x3; // Map the last page to itself for recursive paging
    load_page_directory((uint32_t *)((uint32_t)kpage_directory - (uint32_t)_virt_offset));
    enable_paging();

    // 设置page空闲列表，直接使用全局数组记录每个page
    page_list = (struct page *)kheap_top;
    for (i = 0; i < ktotal_pages; i++) {
        page_list[i].base = (uint32_t *)(i * 0x1000);
        page_list[i].flags = PAGE_UNUSED;
        page_list[i].kref = 0;
    }

    kheap_top += (sizeof(struct page) * ktotal_pages) + (4096 - ((sizeof(struct page) * ktotal_pages) % 4096));
    // 将内核已覆盖的内存页的标志改为已使用
    for (i = 0; i < (kheap_top - (uint32_t)_kernel_base) / 0x1000; i++) {
        page_list[i].flags &= PAGE_USED;
        page_list[i].kref = 1;
    }
    cga_printf("kheap_top %X\n", kheap_top);
    return 0;
}

struct page *alloc_page(void)
{
    struct page *fp = NULL;
    for (fp = page_list; (uint32_t)fp->base < (uint32_t)_virt_offset + host_total_mem; fp++) {
        if (fp->flags == PAGE_UNUSED) {
            fp->flags = PAGE_USED;
            fp->kref = 0;
            return fp;
        }
    }
}

void free_page(struct page *page)
{
    if (page->kref <= 0) page->flags = PAGE_UNUSED;
    else page->kref--;
}

void *get_physaddr(void *virtualaddr)
{
    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

    unsigned long *pd = (unsigned long *)0xFFFFF000;
    // Here you need to check whether the PD entry is present.
    if ((pd[pdindex] & 0x1) == 0) return NULL;

    unsigned long *pt = ((unsigned long *)0xFFC00000) + (0x400 * pdindex);
    // Here you need to check whether the PT entry is present.
    if ((pt[ptindex] & 0x1) == 0) return NULL;

    return (void *)((pt[ptindex] & ~0xFFF) + ((unsigned long)virtualaddr & 0xFFF));
}

void map_page(void *physaddr, void *virtualaddr, unsigned int flags)
{
    // Make sure that both addresses are page-aligned.

    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

    unsigned long *pd = (unsigned long *)0xFFFFF000;
    // Here you need to check whether the PD entry is present.
    // When it is not present, you need to create a new empty PT and
    // adjust the PDE accordingly.
    if ((pd[pdindex] & 0x1) == 0) {
        struct page *t;
        t = alloc_page();
        t->kref = 1;
        pd[pdindex] = (uint32_t)t->base | 0x3;
    }

    unsigned long *pt = ((unsigned long *)0xFFC00000) + (0x400 * pdindex);
    // Here you need to check whether the PT entry is present.
    // When it is, then there is already a mapping present. What do you do now?
    if ((pt[ptindex] & 0x1)) return;

    physaddr = (uint32_t)physaddr & 0xfffff000;
    pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFF) | 0x01; // Present

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