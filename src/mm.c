#include "mm.h"
#include "paging.h"
#include "multiboot2.h"
#include "panic.h"
#include "libc/stdlib.h"
#include "list.h"
#include "aligned.h"

#define EARLY_MEM_POOL_LEN (128 * 1024)

ALIGN_ATTR(4096)
static uint8_t early_mem_pool[EARLY_MEM_POOL_LEN];
static uint8_t *early_free_ptr = NULL;
extern uint32_t _kernel_virt_end_aligned[];
LIST_HEAD(mm_list);
LIST_HEAD(mm_free_list);

void early_mm_init(void)
{
    early_free_ptr = early_mem_pool;
}

void mm_init(void)
{
    uint32_t i;
    uint32_t global_pages;
    uint32_t avail_page;
    struct page *pg;
    struct mm_area *first_mm;

    first_mm = (struct mm_area *)early_malloc(sizeof(struct mm_area));

    // kmalloc的起始地址在page list后面
    global_pages = (get_total_mem() / 4096);
    avail_page = global_pages / 8;

    // alloc kmalloc cache
    // max 512MB
    for (i = 0; i < avail_page; i++)
    {
        pg = alloc_page();
        if (pg == NULL)
        {
            PANIC("MM error\n");
        }
    }
    // uint32_t kheap_begin from paging.c
    first_mm->start = kheap_begin;
    first_mm->size = i * 0x1000;
    list_add_tail(&first_mm->list, &mm_free_list);
}

void *early_malloc(size_t len)
{
    void *start = (void *)early_free_ptr;
    len = ALIGN_4K(len);
    early_free_ptr = (uint8_t *)((size_t)early_free_ptr + len);
    return start;
}

void *kmalloc(size_t len)
{
    struct list_head *pos;
    struct mm_area *m;
    void *start;

    len = ALIGN_4B(len);
    list_for_each(pos, &mm_free_list)
    {
        m = container_of(pos, struct mm_area, list);
        if (m->size >= (len + sizeof(struct mm_area)))
        {
            break;
        }
    }
    start = (void *)m->start;
    m->start = m->start + len + sizeof(struct mm_area);
    m->size -= len + sizeof(struct mm_area);

    // place it after data
    // setup new mm_area struct
    m = (struct mm_area *)((size_t)start + len);
    m->start = (uint32_t)start;
    m->size = len;
    list_add_tail(&m->list, &mm_list);
    return start;
}

void *kmalloc_aligned(size_t len)
{
    struct mm_area *m;
    struct list_head *pos;

    len = ALIGN_4K(len);
    list_for_each(pos, &mm_free_list)
    {
        m = container_of(pos, struct mm_area, list);
        if (m->size >= (len + sizeof(struct mm_area)))
        {
            break;
        }
    }
    m->start = ALIGN_4K(m->start);
    return kmalloc(len);
}

void kfree(void *start)
{
    struct list_head *pos;
    struct mm_area *m;
    list_for_each(pos, &mm_list)
    {
        m = container_of(pos, struct mm_area, list);
        if (m->start == (uint32_t)start)
        {
            list_del(&m->list);
            list_add(&m->list, &mm_free_list);
            break;
        }
    }
}