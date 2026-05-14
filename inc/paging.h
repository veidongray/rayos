#ifndef PAGING_H
#define PAGING_H

#include "list.h"
#include <stdint.h>

#define flush_tlb()                \
    do                             \
    {                              \
        asm volatile(              \
            "mov %cr3, %eax\r\n"   \
            "mov %eax, %cr3\r\n"); \
    } while (0)

struct page
{
    int kref;
    uint32_t *base;
    struct list_head list;
};

extern struct page *page_list;
extern void load_page_directory(uint32_t *page_directory);
extern void enable_paging();
extern uint32_t kheap_begin;

int page_init(void);
int map_page_range(void *physaddr, void *virtualaddr, unsigned int flags, size_t len);
int unmap_page_range(void *virtualaddr, size_t len);
int map_page(void *physaddr, void *virtualaddr, unsigned int flags);
int unmap_page(void *virtualaddr);
struct page *alloc_page(void);
void free_page(struct page *page);
void get_cr3(uint32_t *cr3);
void early_page_init(void);
void *get_physaddr(void *virtualaddr);
void copy_kernel_pagedir(uint32_t *pd);

#endif // PAGING_H