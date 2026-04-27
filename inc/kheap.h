#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>

#define KHEAP_ALLOC 0x0

struct heap_block
{
    uint32_t *start;
    uint32_t *end;
    uint32_t size;
    struct heap_block *next;
} __attribute__((packed));

void kheap_init(void);
void *kmalloc(uint32_t size, uint32_t flag);
void kfree(void *ptr);
void *kcreate_heap_pool(uint32_t *start, uint32_t size);

#endif // KHEAP_H