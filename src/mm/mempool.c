#include <mempool.h>
#include <page.h>

void pool_init(struct mem_pool *pool, void *page, size_t nr_pages,
               size_t obj_size)
{
	pool->obj_size = obj_size;
	pool->total = (nr_pages * PAGE_SIZE) / obj_size;
	pool->used = 0;
	pool->free_list = NULL;

	/* 从后往前链，保证分配顺序从低地址到高地址 */
	for (int i = pool->total - 1; i >= 0; i--) {
		void **slot = (void **)((char *)page + i * obj_size);
		*slot = pool->free_list;
		pool->free_list = slot;
	}
}

void *pool_alloc(struct mem_pool *pool)
{
	if (!pool->free_list)
		return NULL;
	void *obj = pool->free_list;
	pool->free_list = *(void **)obj;
	pool->used++;
	return obj;
}

void pool_free(struct mem_pool *pool, void *obj)
{
	*(void **)obj = pool->free_list;
	pool->free_list = obj;
	pool->used--;
}