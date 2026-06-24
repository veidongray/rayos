#include <mm.h>
#include <page.h>
#include <types.h>
#include <printk.h>
#include <mempool.h>

#define VMAP_AREA_POOL_NR_PAGES 8
#define KMEM_BASE 0xffff900000000000ULL

static struct mem_pool vmap_area_pool;
static LIST_HEAD(vmap_area_list);

static struct vmap_area *__find_vmap_area(__u64 va)
{
    struct vmap_area *vm;
    struct list_head *pos;

    list_for_each(pos, &vmap_area_list)
    {
        vm = container_of(pos, struct vmap_area, list);
        if (vm->va_start == va)
            return vm;
    }
    return NULL;
}

void *kmalloc(size_t size)
{
    int ret_map;
    int nr_order;
    __u64 va;
    __u64 pa;
    struct vmap_area *vm;

    if (size == 0)
        return NULL;

    nr_order = size_to_order(size + sizeof(struct vmap_area));
    pa = alloc_pages(nr_order);
    if ((int64_t)pa < 0)
        return NULL;

    va = KMEM_BASE + pa;
    ret_map = map_page_range(pa, va, MAP_KERN_RW, order_to_pages(nr_order));
    if ((uint64_t)ret_map < order_to_pages(nr_order))
        return NULL;

    vm = (struct vmap_area *)pool_alloc(&vmap_area_pool);
    printk("alloc vm %#llx\n", vm);
    vm->va_start = va;
    vm->va_nrpages = order_to_pages(nr_order);
    vm->va_end = va + (vm->va_nrpages * PAGE_SIZE);
    list_add_tail(&vm->list, &vmap_area_list);

    return (void *)vm->va_start;
}

void *kzalloc(size_t size)
{
    size_t nr;
    uint8_t *ret_ptr;

    ret_ptr = (uint8_t *)kmalloc(size);
    if (ret_ptr == NULL)
        return ret_ptr;
    for (nr = 0; nr < size; nr++)
    {
        // clear mem
        ret_ptr[nr] = 0x0;
    }
    return (void *)ret_ptr;
}

void kfree(void *virtaddr)
{
    size_t order;
    uint64_t physaddr;
    struct vmap_area *vm;

    vm = __find_vmap_area((__u64)virtaddr);
    printk("Free vm %#llx\n", vm);
    list_del(&vm->list);
    order = size_to_order(vm->va_nrpages * PAGE_SIZE);
    physaddr = get_physaddr((uint64_t)virtaddr);
    free_pages(physaddr, order);
    unmap_page_range((uint64_t)virtaddr, vm->va_nrpages);
    pool_free(&vmap_area_pool, vm);
}

void mm_init(void)
{
    __u64 pa, va;

    pa = alloc_pages(pages_to_order(VMAP_AREA_POOL_NR_PAGES));
    va = pa + KMEM_BASE;

    /**
     * 创建 vmap_area 元数据的 mempool
     */
    map_page_range(pa, va, MAP_KERN_RW, VMAP_AREA_POOL_NR_PAGES);
    pool_init(&vmap_area_pool, va, VMAP_AREA_POOL_NR_PAGES, sizeof(struct vmap_area));
}