#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE 0x1000ULL
#define PML4_BASE 0xFFFFFFFFFFFFFULL
#define KERNEL_BASE 0xffff800000000000ULL

/* 基础属性位 */
#define PTE_PRESENT (1ULL << 0)  // 页面有效
#define PTE_WRITABLE (1ULL << 1) // 可写
#define PTE_USER (1ULL << 2)     // 用户态可访问
#define PTE_PWT (1ULL << 3)      // Write-Through
#define PTE_PCD (1ULL << 4)      // Cache Disable
#define PTE_ACCESSED (1ULL << 5) // 已访问（CPU自动置位）
#define PTE_DIRTY (1ULL << 6)    // 已修改（仅PT级，CPU自动置位）
#define PTE_HUGE (1ULL << 7)     // 大页（PDPT=1G, PD=2M）
#define PTE_GLOBAL (1ULL << 8)   // 全局页（跳过TLB刷新）
#define PTE_NX (1ULL << 63)      // 禁止执行

/* 常用组合标志 */
#define MAP_KERN_RW (PTE_PRESENT | PTE_WRITABLE)
#define MAP_KERN_RO (PTE_PRESENT)
#define MAP_KERN_MMIO (PTE_PRESENT | PTE_WRITABLE | PTE_PCD | PTE_PWT)
#define MAP_USER_RW (PTE_PRESENT | PTE_WRITABLE | PTE_USER)
#define MAP_USER_RO (PTE_PRESENT | PTE_USER)

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

// number of pages -> order
static inline uint64_t pages_to_order(size_t nr_pages)
{
    return size_to_order(nr_pages * PAGE_SIZE);
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

size_t memused(void);
size_t memused_percent(void);

#endif // PAGE_H