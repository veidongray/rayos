#include "mm.h"
#include "paging.h"
#include "multiboot2.h"
#include "panic.h"
#include "libc/stdlib.h"

#define KMEM_POOL_LEN (16 * 1024 * 1024)
#define EARLY_MEM_POOL_LEN (4 * 1024 * 1024)

static uint8_t *early_free_ptr = NULL;
static uint8_t *kfree_ptr = NULL;
static uint8_t early_mem_pool[EARLY_MEM_POOL_LEN] __attribute__((aligned(4096)));
LIST_HEAD(mm_list);
extern uint32_t _kernel_virt_end_aligned[];
extern uint32_t _mboot_info[];
extern uint32_t _mboot_magic[];

void early_mm_init(void)
{
    if (_mboot_magic[0] == 0x36d76289)
    {
        parse_multiboot2_mmap((uint32_t *)_mboot_info[0]);
    }
    else
    {
        // If we don't have a valid multiboot magic number, we can't trust the bootloader and should halt
        PANIC("Lost Bootloader...\n");
    }
    early_free_ptr = early_mem_pool;
}

void mm_init(void)
{
    uint32_t i;
    struct page *pg;

    kfree_ptr = (uint8_t *)_kernel_virt_end_aligned;
    for (i = 0; i < KMEM_POOL_LEN; i += 0x1000)
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
    struct mm_area *m = (struct mm_area *)kfree_ptr;
    void *ptr = (void *)((size_t)kfree_ptr + sizeof(struct mm_area));

    len = ((len % 4) == 0) ? len : (len - (len % 4) + 4);

    kfree_ptr = (uint8_t *)((size_t)kfree_ptr + len + sizeof(struct mm_area));
    m->start = (uint32_t)ptr;
    m->end = (uint32_t)ptr + len;
    m->size = m->end - m->start;
    list_add_tail(&m->list, &mm_list);
    return ptr;
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