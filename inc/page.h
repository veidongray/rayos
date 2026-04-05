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
uint32_t *get_physaddr(uint32_t *virtaddr);

#endif // PAGE_H