#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <types.h>
#include <stddef.h>

struct mem_pool
{
    void *free_list; /* 空闲对象头指针 */
    size_t obj_size;
    size_t total;
    size_t used;
};

void pool_init(struct mem_pool *pool, void *page, size_t nr_pages, size_t obj_size);
void *pool_alloc(struct mem_pool *pool);
void pool_free(struct mem_pool *pool, void *obj);

#endif // MEMPOOL_H