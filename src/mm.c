#include <mm.h>
#include <page.h>

#define KMEM_BASE 0xffff900000000000ULL

void *kmalloc(size_t size)
{
    int ret_map;
    int nr_order;
    uint64_t ret_physaddr;
    struct vmap_area *vm;

    if (size == 0)
        return NULL;

    nr_order = size_to_order(size + sizeof(struct vmap_area));
    ret_physaddr = alloc_pages(nr_order);
    if ((int64_t)ret_physaddr < 0)
        return NULL;

    ret_map = map_page_range(ret_physaddr, KMEM_BASE + ret_physaddr, 0x3, 0x1 << nr_order);
    if (ret_map < (0x1 << nr_order))
        return NULL;

    vm = (struct vmap_area *)(KMEM_BASE + ret_physaddr);
    vm->va_start = KMEM_BASE + ret_physaddr + sizeof(struct vmap_area);
    vm->va_end = KMEM_BASE + ret_physaddr + ((0x1 << nr_order) * PAGE_SIZE);
    vm->va_nrpages = 0x1 << nr_order;
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
    struct vmap_area *vm;

    vm = (struct vmap_area *)(virtaddr - sizeof(struct vmap_area));
    unmap_page_range((uint64_t)virtaddr, vm->va_nrpages);
}

uint64_t size_to_order(size_t size)
{
    if (size == 0)
        return 0;
    size_t pages = (size + (1UL << PAGE_SHIFT) - 1) >> PAGE_SHIFT;
    return fls(pages - 1);
}