#include "mm.h"
#include "paging.h"
#include "multiboot2.h"
#include "panic.h"
#include "libc/stdlib.h"

#define EARLY_MEM_POOL_LEN (2 * 1024 * 1024)

static uint8_t *early_free_ptr = NULL;
static uint8_t *kfree_ptr = NULL;
static uint8_t early_mem_pool[EARLY_MEM_POOL_LEN] __attribute__((aligned(4096)));
LIST_HEAD(mm_list);
extern uint32_t _kernel_virt_end_aligned[];

void early_mm_init(void)
{
    early_free_ptr = early_mem_pool;
}

void mm_init(void)
{
    uint32_t i, free_pages;
    struct page *pg;

    free_pages = (get_total_mem() / 4096) > 0x40000 ? 0x40000 : (get_total_mem() / 4096);
    kfree_ptr = (uint8_t *)(_kernel_virt_end_aligned + (free_pages * sizeof(struct page)));
    free_pages -= ((uint32_t)kfree_ptr - 0xc0000000) / 4096;
    
    // alloc kmalloc cache
    for (i = 0; i < free_pages; i += 0x1000)
    {
        pg = alloc_page();
        map_page(pg->base, (uint32_t *)((uint32_t)kfree_ptr + i), 0x3);
    }
}

void *early_malloc(size_t len)
{
    void *ptr = (void *)early_free_ptr;
    len = ((len % 4) == 0) ? len : (len - (len % 4) + 4);
    early_free_ptr = (uint8_t *)((size_t)early_free_ptr + len);
    return ptr;
}

void *kmalloc(size_t len)
{
    struct mm_area *m;
    void *ptr;

    ptr = (void *)kfree_ptr;
    len = ((len % 4) == 0) ? len : (len - (len % 4) + 4);
    // place it after data
    m = (struct mm_area *)((size_t)kfree_ptr + len);

    kfree_ptr = (uint8_t *)((size_t)kfree_ptr + len + sizeof(struct mm_area));
    m->start = (uint32_t)ptr;
    m->size = len;
    list_add_tail(&m->list, &mm_list);
    return ptr;
}

void *kmalloc_aligned(size_t len)
{
    if ((uint32_t)kfree_ptr % 4096) {
        // aligned 4K
        kfree_ptr = (uint8_t *)((uint32_t)kfree_ptr - ((uint32_t)kfree_ptr % 4096) + 4096);
    }

    return kmalloc(len);
}

void kfree(void *ptr)
{
    struct list_head *pos;
    struct mm_area *m;
    list_for_each(pos, &mm_list)
    {
        m = container_of(pos, struct mm_area, list);
        if (m->start == (uint32_t)ptr)
        {
            list_del(&m->list);
            break;
        }
    }
}