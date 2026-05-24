#ifndef PAGE_H
#define PAGE_H

#include <types.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE 0x1000ULL
#define KERNEL_BASE 0xffff800000000000ULL

static inline int fls(int x)
{
    // 返回最高位位置 (GCC 内建函数)
    return x ? 32 - __builtin_clz(x) : 0;
}

int unmap_page_range(uint64_t virtaddr, size_t len);
int map_page_range(uint64_t physaddr, uint64_t virtaddr, uint64_t flags, size_t len);
void page_init(void);
void free_pages(uint64_t physaddr, size_t len);
void load_pml4(uint64_t pml4_physaddr);
uint64_t get_cr3(void);
uint64_t alloc_pages(size_t order);
uint64_t size_to_order(size_t size);
uint64_t order_to_pages(size_t order);
uint64_t get_physaddr(uint64_t virtaddr);

#define alloc_page() alloc_pages(0)
#define free_page(physaddr) free_pages(physaddr, 0)
#define unmap_page(virtaddr) unmap_page_range(virtaddr, 1)
#define map_page(physaddr, virtaddr, flags) map_page_range(physaddr, virtaddr, flags, 1)

#endif // PAGE_H