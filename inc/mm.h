#ifndef MM_H
#define MM_H

#include <stdint.h>
#include <stddef.h>
#include "list.h"

struct mm_area
{
    uint32_t start;
    size_t size;
    struct list_head list;
};

void *early_malloc(size_t len);
void *kmalloc(size_t len);
void *kmalloc_aligned(size_t len);
void kfree(void *ptr);
void early_mm_init(void);
void mm_init(void);

#endif // MM_H