#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

extern uint32_t kheap_top;

struct page {
    uint32_t *base;
    int32_t kref;
    uint32_t flags;
} __attribute__((packed));

int page_init(void);
void *get_physaddr(void *virtualaddr);
void flush_tlb(void);
void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
struct page *alloc_page(void);
void free_page(struct page *page);

#endif // PAGING_H