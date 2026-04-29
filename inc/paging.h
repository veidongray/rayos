#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

struct page {
    uint32_t *base;
    int32_t kref;
    uint32_t flags;
} __attribute__((packed));

extern uint32_t kheap_top;
extern struct page *page_list;
extern uint32_t kpage_directory[1024] __attribute__((aligned(4096)));
extern void load_page_directory(uint32_t *page_directory);
extern void enable_paging();

int page_init(void);
void *get_physaddr(void *virtualaddr);
void flush_tlb(void);
void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
struct page *alloc_page(void);
void free_page(struct page *page);

#endif // PAGING_H