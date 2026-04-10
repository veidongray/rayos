#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

extern uint32_t _kernel_start[];
extern uint32_t _kernel_end[];
extern uint32_t _kernel_end_aligned[];

typedef uint32_t *pd_t;
typedef uint32_t *pt_t;
typedef uint32_t *pg_t;
typedef uint32_t *physaddr_t;
typedef uint32_t *virtaddr_t;

extern physaddr_t heap_top;

// This should go outside any function..
extern void load_page_directory(uint32_t *);
extern void enable_paging();
int page_init(void);
physaddr_t get_physaddr(virtaddr_t virtaddr);
physaddr_t alloc_page(void);
virtaddr_t alloc_pages(uint32_t num);
int map_page(uint32_t *physaddr, uint32_t *virtaddr, uint32_t flags);
int unmap_page(virtaddr_t virtaddr);
int free_page(virtaddr_t virtaddr);
int flush_tlb(void);
uint8_t get_bitmap(uint8_t *bm, uint32_t index);
uint8_t set_bitmap(uint8_t *bm, uint32_t index);
uint8_t clr_bitmap(uint8_t *bm, uint32_t index);

#endif // PAGE_H