#include <stdint.h>
#include "print.h"

// from paging.c
extern uint32_t *kheap_pool_start;
struct heap_block {
    uint32_t *start;
    uint32_t *end;
    uint32_t size;
    struct heap_block *next;
};

static struct heap_block *kheap_pool = 0;

void kheap_init(void)
{
    uint32_t i;

    // Create a empty block for pool
    kheap_pool = (struct heap_block *)kheap_pool_start; // minimum sizeof(struct heap_block) + 32 bytes each time
    kheap_pool->start = (uint32_t *)((uint32_t)kheap_pool_start + sizeof(struct heap_block)); // Start the first block right after the heap pool structure
    kheap_pool->end = (uint32_t *)((uint32_t)kheap_pool_start + sizeof(struct heap_block)); // Let's say we reserve 4MB for the heap
    kheap_pool->size = 0x400000 - sizeof(struct heap_block); // Total size minus the heap block structure
    kheap_pool->next = 0;

    cga_printf("sizeof(struct heap_block) %u\n", sizeof(struct heap_block));
    cga_printf("kheap_pool %X\n", kheap_pool);
}

void *kmalloc(uint32_t size)
{
    void *addr = (void *)0;
    struct heap_block *ptr = kheap_pool;
    struct heap_block *new_hb = 0;

    cga_printf("struct heap_block *kheap_pool %X\n", kheap_pool);
    if (size == 0) return addr;
    size += ((size % 32) == 0)? 0: 32; // aligned 32 bytes

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