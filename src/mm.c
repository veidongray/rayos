#include "mm.h"
#include "paging.h"
#include "multiboot2.h"
#include "panic.h"
#include "libc/stdlib.h"
#include "list.h"
#include "aligned.h"

#define EARLY_MEM_POOL_LEN (2 * 1024 * 1024)

static uint8_t *early_free_ptr = NULL;
static uint8_t *kmalloc_ptr = NULL;
static uint8_t early_mem_pool[EARLY_MEM_POOL_LEN] __attribute__((aligned(4096)));
LIST_HEAD(mm_list);
extern uint32_t _kernel_virt_end_aligned[];

void early_mm_init(void)
{
    early_free_ptr = early_mem_pool;
}

void mm_init(void)
{
    uint32_t i, global_pages, avail_page;
    struct page *pg;

    // kmalloc的起始地址在page list后面
    global_pages = (get_total_mem() / 4096);
    kmalloc_ptr = (uint8_t *)(_kernel_virt_end_aligned + (global_pages * sizeof(struct page)));
    avail_page = global_pages / 8;

    // alloc kmalloc cache
    for (i = 0; i < avail_page; i++)
    {
        pg = alloc_page();
        if (pg == NULL)
            break;
    }
}

void *early_malloc(size_t len)
{
    void *ptr = (void *)early_free_ptr;
    len = ALIGN_4B(len);
    early_free_ptr = (uint8_t *)((size_t)early_free_ptr + len);
    return ptr;
}

void *kmalloc(size_t len)
{
    struct mm_area *m;
    void *ptr;

    ptr = (void *)kmalloc_ptr;
    len = ALIGN_4B(len);
    // place it after data
    m = (struct mm_area *)((size_t)kmalloc_ptr + len);

    kmalloc_ptr = (uint8_t *)((size_t)kmalloc_ptr + len + sizeof(struct mm_area));
    m->start = (uint32_t)ptr;
    m->size = len;
    list_add_tail(&m->list, &mm_list);
    return ptr;
}

void *kmalloc_aligned(size_t len)
{
    kmalloc_ptr = (uint8_t *)ALIGN_4K((uint32_t)kmalloc_ptr);
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