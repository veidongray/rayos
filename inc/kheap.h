#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>

struct heap_block {
    uint32_t *start;
    uint32_t *end;
    uint32_t size;
    struct heap_block *next;
} __attribute__((packed));

void kheap_init(void);
void *kmalloc(uint32_t size);
void kfree(void *ptr);

#endif // KHEAP_H