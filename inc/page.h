#ifndef PAGE_H
#define PAGE_H

#include <types.h>

int unmap_page_range(uint64_t virtaddr, size_t len);
int map_page_range(uint64_t physaddr, uint64_t virtaddr, uint64_t flags, size_t len);
void page_init(void);
void free_pages(uint64_t physaddr, size_t len);
void load_pml4(uint64_t pml4_physaddr);
uint64_t get_cr3(void);
uint64_t alloc_pages(size_t order);
uint64_t get_physaddr(uint64_t virtaddr);

#define alloc_page() alloc_pages(0)
#define free_page(virtaddr) free_pages(virtaddr, 0)
#define unmap_page(virtaddr) unmap_page_range(virtaddr, 1)
#define map_page(physaddr, virtaddr, flags) map_page_range(physaddr, virtaddr, flags, 1)

#endif // PAGE_H