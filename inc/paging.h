#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

extern uint32_t kheap_top;

int page_init(void);
void *get_physaddr(void *virtualaddr);
void flush_tlb(void);
void map_page(void *physaddr, void *virtualaddr, unsigned int flags);

#endif // PAGING_H