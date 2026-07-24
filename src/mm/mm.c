#include <mempool.h>
#include <mm.h>
#include <page.h>
#include <printk.h>
#include <spinlock.h>
#include <string.h>
#include <types.h>

#define VMAP_AREA_POOL_NR_PAGES 32 /* vmap_area 元数据池占用的页数 */
#define KMEM_BASE 0xffff900000000000ULL /* 内核动态映射区虚拟基地址 */

static struct mem_pool vmap_area_pool; /* vmap_area 对象池 */
static LIST_HEAD(vmap_area_list);      /* 已分配 vmap_area 全局链表 */

static spinlock_t mm_spinlock; // 内存分配和释放的自旋锁

static struct vmap_area *__vmap_area_alloc(__u64 start, __u64 nr_pages)
{
	/* 从池中分配元数据并挂入全局链表 */
	struct vmap_area *vm;
	vm = (struct vmap_area *)pool_alloc(&vmap_area_pool);
	if (!vm)
		return NULL;
	vm->va_start = start;
	vm->va_nrpages = nr_pages;
	vm->va_end = start + (vm->va_nrpages * PAGE_SIZE);
	list_add_tail(&vm->list, &vmap_area_list);
	return vm;
}

static __u64 __kmem_map(__u64 nr_order)
{
	int ret;
	__u64 pa, va;

	pa = alloc_pages(nr_order);
	if ((int64_t)pa < 0)
		return 0;

	/* 建立物理页到 KMEM_BASE 区域的虚拟映射 */
	va = KMEM_BASE + pa;
	ret = map_page_range(pa, va, MAP_KERN_RW, order_to_pages(nr_order));
	if ((__u64)ret < order_to_pages(nr_order))
		return 0;

	return va;
}

/* 根据虚拟地址在链表中查找对应的 vmap_area */
static struct vmap_area *__find_vmap_area(__u64 va)
{
	struct vmap_area *vm;

	list_for_each_entry(vm, &vmap_area_list, list)
	{
		if (vm->va_start == va)
			return vm;
	}
	return NULL;
}

/* 分配内核内存：物理页 + 虚拟映射 + 元数据记录 */
void *kmalloc(size_t size)
{
	int nr_order;
	__u64 va;
	struct vmap_area *vm;

	spinlock_lock(&mm_spinlock);
	if (size == 0)
		return NULL;

	/* 计算所需页数并分配物理内存 */
	nr_order = size_to_order(size);
	va = __kmem_map(nr_order);
	if (va == 0)
		return NULL;

	vm = __vmap_area_alloc(va, order_to_pages(nr_order));
	if (vm == NULL)
		return NULL;

	spinlock_unlock(&mm_spinlock);
	return (void *)vm->va_start;
}

/* 分配并清零内核内存 */
void *kzalloc(size_t size)
{
	__u8 *ret_ptr;

	ret_ptr = (__u8 *)kmalloc(size);
	if (ret_ptr == NULL)
		return ret_ptr;

	memset(ret_ptr, 0, size);

	return (void *)ret_ptr;
}

/* 释放内核内存：解映射 + 归还物理页 + 归还元数据 */
int kfree(void *virtaddr)
{
	size_t order;
	__u64 physaddr;
	struct vmap_area *vm;

	spinlock_lock(&mm_spinlock);
	/* 通过虚拟地址反查元数据 */
	vm = __find_vmap_area((__u64)virtaddr);
	if (!vm) {
		return -1;
	}

	list_del(&vm->list);

	/* 按分配时的 order 释放物理页 */
	order = size_to_order(vm->va_nrpages * PAGE_SIZE);
	physaddr = get_physaddr((__u64)virtaddr);
	free_pages(physaddr, order);

	/* 解除虚拟映射并归还元数据到对象池 */
	unmap_page_range((__u64)virtaddr, vm->va_nrpages);
	pool_free(&vmap_area_pool, vm);

	spinlock_unlock(&mm_spinlock);
	return 0;
}

void *krealloc(void *ptr, size_t new_size)
{
	void *new;

	if (!ptr)
		return NULL;

	new = kmalloc(new_size);
	if (!new)
		return NULL;

	memcpy(new, ptr, new_size);

	kfree(ptr);
	return new;
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
	pool_init(&vmap_area_pool, (void *)va, VMAP_AREA_POOL_NR_PAGES,
	          sizeof(struct vmap_area));

	spinlock_init(&mm_spinlock);
}