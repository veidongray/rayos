#ifndef PAGE_H
#define PAGE_H

void page_init(void);
void load_pml4(uint64_t pml4_physaddr);

#endif // PAGE_H