#ifndef MM_H
#define MM_H

#include <types.h>

struct vmap_area
{
    uint64_t va_start;
    uint64_t va_end;
    uint64_t va_nrpages;
};

static inline int fls(int x)
{
    // 返回最高位位置 (GCC 内建函数)
    return x ? 32 - __builtin_clz(x) : 0;
}

void *kmalloc(size_t size);
void *kzalloc(size_t size);
void kfree(void *virtaddr);
uint64_t size_to_order(size_t size);

#endif // MM_H