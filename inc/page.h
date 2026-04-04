#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

extern uint32_t _kernel_start[];
extern uint32_t _kernel_end[];
extern uint32_t _kernel_end_aligned[];

typedef uint32_t pd_t;
typedef uint32_t pt_t;

// This should go outside any function..
extern void load_page_directory(uint32_t *);
extern void enable_paging();
int page_init(void);
int page_set_rw(uint32_t *page);
int page_set_present(uint32_t *page);
uint32_t page_get_physaddr(uint32_t virtualaddr);

#endif // PAGE_H