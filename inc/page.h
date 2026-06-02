#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE 0x1000ULL
#define PML4_BASE 0xFFFFFFFFFFFFFULL
#define KERNEL_BASE 0xffff800000000000ULL

#define order_to_pages(order) (0x1ULL << (order))

// 找出一个非零数的最高位位置（1-based），等价于 fls
static inline int fls(unsigned long x)
{
    int r = 0;
    while (x)
    {
        x >>= 1;
        r++;
    }
    return r;
}

// size -> order
static inline uint64_t size_to_order(size_t size)
{
    if (size == 0)
        return 0;

    // 向上取页
    size_t pages = (size + PAGE_SIZE - 1) >> PAGE_SHIFT;

    // 返回阶数
    return fls(pages - 1);
}

int unmap_page_range(uint64_t virtaddr, size_t len);
int map_page_range(uint64_t physaddr, uint64_t virtaddr, uint64_t flags, size_t len);
void page_init(void);
void free_pages(uint64_t physaddr, size_t order);
void load_pml4(uint64_t pml4_physaddr);
uint64_t alloc_pages(size_t order);
uint64_t get_physaddr(uint64_t virtaddr);

#define alloc_page() alloc_pages(0)
#define free_page(physaddr) free_pages(physaddr, 0)
#define unmap_page(virtaddr) unmap_page_range(virtaddr, 1)
#define map_page(physaddr, virtaddr, flags) map_page_range(physaddr, virtaddr, flags, 1)

#endif // PAGE_H