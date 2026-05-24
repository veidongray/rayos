#ifndef MM_H
#define MM_H

#include <types.h>

struct vmap_area
{
    uint64_t va_start;
    uint64_t va_end;
    uint64_t va_nrpages;
};

void *kmalloc(size_t size);
void *kzalloc(size_t size);
void kfree(void *virtaddr);

#endif // MM_H