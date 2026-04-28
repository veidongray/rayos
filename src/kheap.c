#include <stdint.h>
#include "print.h"
#include "paging.h"
#include "kheap.h"
#include "multiboot2.h"
#include "libc/stdlib.h"

extern uint32_t _virt_offset[];
static struct heap_block *kheap_pool = NULL;

void kheap_init(void)
{
    kheap_pool = (struct heap_block *)kcreate_heap_pool((uint32_t *)kheap_top, KHEAP_SIZE);
    // 在这里之后不再使用kheap_top直接分配空间
    // 而是使用kmalloc和kfree在分配到的kheap_pool中进行内存管理
    kheap_top += KHEAP_SIZE + 0x1000;
    if (kheap_pool == NULL)
    {
        cga_printf("Host desn't have enough memory space...\n");
        while (1)
            asm volatile("hlt\r\n");
    }
}

void *kmalloc(uint32_t size, uint32_t flag)
{
    void *addr = NULL;
    if (flag == KHEAP_ALLOC)
    {
        // alloc mamory area from kheap space
        struct heap_block *ptr = kheap_pool;
        struct heap_block *new_hb = 0;

        if (size == 0)
            return NULL;
        size += ((size % 32) == 0) ? 0 : 32; // aligned 32 bytes
        if (size + sizeof(struct heap_block) + (uint32_t)kheap_pool->end > kheap_pool->size + _virt_offset[0])
            return NULL;

        // Find empty memory
        for (; ptr->next != 0; ptr = ptr->next)
        {
            // found a enough space between heap_block list
            if (((uint32_t)ptr->next - (uint32_t)ptr->end) > (size + sizeof(struct heap_block)))
            {
                new_hb = (struct heap_block *)ptr->end;
                new_hb->start = (uint32_t *)((uint32_t)ptr->end + sizeof(struct heap_block));
                new_hb->end = (uint32_t *)((uint32_t)ptr->end + sizeof(struct heap_block) + size);
                new_hb->size = size;
                new_hb->next = ptr->next;
                ptr->next = new_hb;
                addr = (void *)new_hb->start;
                return addr;
            }
        }

        // if doesn't have enough space between heap_block nodes
        // then append new
        new_hb = (struct heap_block *)ptr->end;
        new_hb->start = (uint32_t *)((uint32_t)ptr->end + sizeof(struct heap_block));
        new_hb->end = (uint32_t *)((uint32_t)ptr->end + sizeof(struct heap_block) + size);
        new_hb->size = size;
        new_hb->next = 0;
        ptr->next = new_hb;
        addr = (void *)new_hb->start;
    }
    return addr;
}

void kfree(void *ptr)
{
    struct heap_block *hb_ptr = kheap_pool;

    while (hb_ptr->next)
    {
        if (hb_ptr->next->start == (uint32_t *)ptr)
        {
            hb_ptr->next = hb_ptr->next->next;
            return;
        }
        hb_ptr = hb_ptr->next;
    }
}

void *kcreate_heap_pool(uint32_t *start, uint32_t size)
{
    struct heap_block *heap_pool;

    if ((uint32_t)start + size > host_total_mem + _virt_offset[0])
        return NULL;
    // Create a empty block for pool
    heap_pool = (struct heap_block *)start;
    heap_pool->start = (uint32_t *)((uint32_t)start + sizeof(struct heap_block)); // first address
    heap_pool->end = (uint32_t *)(heap_pool->start);                              // end address
    heap_pool->size = size;                                                       // Total size without heap_block structure
    heap_pool->next = 0;
    return heap_pool;
}