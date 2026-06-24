#include <mm.h>
#include <page.h>
#include <types.h>
#include <printk.h>
#include <mempool.h>

#define VMAP_AREA_POOL_NR_PAGES 8       /* vmap_area 元数据池占用的页数 */
#define KMEM_BASE 0xffff900000000000ULL /* 内核动态映射区虚拟基地址 */

static struct mem_pool vmap_area_pool;  /* vmap_area 对象池 */
static LIST_HEAD(vmap_area_list);       /* 已分配 vmap_area 全局链表 */

/* 根据虚拟地址在链表中查找对应的 vmap_area */
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

/* 分配内核内存：物理页 + 虚拟映射 + 元数据记录 */
void *kmalloc(size_t size)
{
    int ret_map;
    int nr_order;
    __u64 va;
    __u64 pa;
    struct vmap_area *vm;

    if (size == 0)
        return NULL;

    /* 计算所需页数并分配物理内存 */
    nr_order = size_to_order(size + sizeof(struct vmap_area));
    pa = alloc_pages(nr_order);
    if ((int64_t)pa < 0)
        return NULL;

    /* 建立物理页到 KMEM_BASE 区域的虚拟映射 */
    va = KMEM_BASE + pa;
    ret_map = map_page_range(pa, va, MAP_KERN_RW, order_to_pages(nr_order));
    if ((uint64_t)ret_map < order_to_pages(nr_order))
        return NULL;

    /* 从池中分配元数据并挂入全局链表 */
    vm = (struct vmap_area *)pool_alloc(&vmap_area_pool);
    vm->va_start = va;
    vm->va_nrpages = order_to_pages(nr_order);
    vm->va_end = va + (vm->va_nrpages * PAGE_SIZE);
    list_add_tail(&vm->list, &vmap_area_list);

    return (void *)vm->va_start;
}

/* 分配并清零内核内存 */
void *kzalloc(size_t size)
{
    size_t nr;
    uint8_t *ret_ptr;

    ret_ptr = (uint8_t *)kmalloc(size);
    if (ret_ptr == NULL)
        return ret_ptr;

    /* 逐字节清零 */
    for (nr = 0; nr < size; nr++)
        ret_ptr[nr] = 0x0;

    return (void *)ret_ptr;
}

/* 释放内核内存：解映射 + 归还物理页 + 归还元数据 */
void kfree(void *virtaddr)
{
    size_t order;
    uint64_t physaddr;
    struct vmap_area *vm;

    /* 通过虚拟地址反查元数据 */
    vm = __find_vmap_area((__u64)virtaddr);
    list_del(&vm->list);

    /* 按分配时的 order 释放物理页 */
    order = size_to_order(vm->va_nrpages * PAGE_SIZE);
    physaddr = get_physaddr((uint64_t)virtaddr);
    free_pages(physaddr, order);

    /* 解除虚拟映射并归还元数据到对象池 */
    unmap_page_range((uint64_t)virtaddr, vm->va_nrpages);
    pool_free(&vmap_area_pool, vm);
}

/* 内存子系统初始化：创建 vmap_area 元数据对象池 */
void mm_init(void)
{
    __u64 pa, va;

    /* 为元数据池分配连续物理页并映射到虚拟地址空间 */
    pa = alloc_pages(pages_to_order(VMAP_AREA_POOL_NR_PAGES));
    va = pa + KMEM_BASE;
    map_page_range(pa, va, MAP_KERN_RW, VMAP_AREA_POOL_NR_PAGES);

    /* 用映射后的虚拟地址初始化固定大小对象池 */
    pool_init(&vmap_area_pool, (void *)va,
              VMAP_AREA_POOL_NR_PAGES, sizeof(struct vmap_area));
}