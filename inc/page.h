#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

extern uint32_t _kernel_start[];
extern uint32_t _kernel_end[];
extern uint32_t _kernel_end_aligned[];

// This should go outside any function..
extern void loadPageDirectory(unsigned int *);
extern void enablePaging();
int page_init(void);

#endif // PAGE_H