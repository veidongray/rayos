#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

struct page {
    int kref;
    uint32_t *base;
} __attribute__((packed));

extern struct page *page_list;
extern void load_page_directory(uint32_t *page_directory);
extern void enable_paging();

int page_init(void);
int map_page(void *physaddr, void *virtualaddr, unsigned int flags);
struct page *alloc_page(void);
void free_page(struct page *page);
void get_cr3(uint32_t *cr3);
void early_page_init(void);
void *get_physaddr(void *virtualaddr);
void flush_tlb(void);
void copy_kernel_pagedir(uint32_t *pd);

#endif // PAGING_H