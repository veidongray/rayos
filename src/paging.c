#include <stdint.h>
#include "multiboot2.h"
#include "paging.h"
#include "tty.h"
#include "libc/stdlib.h"
#include "mm.h"
#include "panic.h"
#include "idt.h"
#include "aligned.h"
#include "spinlock.h"
#include <stddef.h>

#define EARLY_PAGES ((16 * 1024 * 1024) / 4096)

extern uint32_t _virt_offset[];
extern uint32_t _kernel_phys_end_aligned[];
extern uint32_t _kernel_virt_end_aligned[];
static uint64_t early_tables[4096] __attribute__((aligned(4096)));
static uint32_t kpage_directory[1024] __attribute__((aligned(4096)));
static uint32_t *kpage_tables = NULL;
static struct page *global_page_list = NULL;
uint32_t kheap_begin;
LIST_HEAD(free_page_list);
LIST_HEAD(used_page_list);
SPINLOCK_INIT(free_page_list_lock);
SPINLOCK_INIT(used_page_list_lock);

void early_page_init(void)
{
    uint64_t i;
    uint64_t cr3, *cr3ptr;
    get_cr3(&cr3);

    for (i = 0; i < 4096; i++)
    {
        early_tables[i] = ((i * 0x1000UL)) | 0x3UL;
    }

    // early map 16MB
    cr3ptr = (uint32_t *)(cr3 + (uint32_t)_virt_offset);
    for (i = 0; i < 4; i++)
    {
        cr3ptr[768 + i] = ((uint32_t)(early_tables + (i * 1024)) - (uint32_t)_virt_offset) | 0x3UL;
    }
    spinlock_init(&free_page_list_lock);
    spinlock_init(&used_page_list_lock);
}

int page_init(void)
{
    uint32_t i;
    uint32_t pages_list_end;
    uint32_t global_pages;
    uint32_t ktotal_pages;
    uint32_t ktotal_tables;

    global_pages = get_total_mem() / 4096;
    // 计算1G以内可用内存的页数和页表数
    // 这个只包括内核高地址最高1G的页数
    ktotal_pages = global_pages > 0x40000 ? 0x40000 : global_pages;
    ktotal_tables = ktotal_pages / 1024;

    // 映射1G以内的系统可用内存空间
    // 从0xC0000000映射到0x00000000开始
    kpage_tables = (uint32_t *)_kernel_virt_end_aligned;
    for (i = 0; i < ktotal_pages; i++)
    {
        kpage_tables[i] = (i * 0x1000UL) | 0x3UL;
    }
    for (i = 0; i < ktotal_tables; i++)
    {
        kpage_directory[i + 768] = ((uint32_t)kpage_tables - (uint32_t)_virt_offset + (i * 0x1000UL)) | 0x3UL;
    }
    kpage_directory[1023] = ((uint32_t)kpage_directory - (uint32_t)_virt_offset) | 0x3UL;
    load_page_directory((uint32_t *)((uint32_t)kpage_directory - (uint32_t)_virt_offset));
    enable_paging();

    // create free page list
    // 记录所有可用内存page
    global_page_list = (struct page *)ALIGN_4K((uint32_t)((uint32_t)_kernel_virt_end_aligned + (ktotal_pages * sizeof(uint32_t))));
    pages_list_end = ALIGN_4K((uint32_t)global_page_list + (global_pages * sizeof(struct page)) - (uint32_t)_virt_offset);
    kheap_begin = pages_list_end + (uint32_t)_virt_offset;
    for (i = 0; i < global_pages; i++)
    {
        global_page_list[i].base = (uint32_t *)(i * 0x1000);
        if ((uint32_t)global_page_list[i].base < pages_list_end)
        {
            // 标记已经被使用的地址
            list_add_tail(&global_page_list[i].list, &used_page_list);
        }
        else
        {
            list_add_tail(&global_page_list[i].list, &free_page_list);
        }
    }

    return 0;
}

struct page *alloc_page(void)
{
    struct page *fp;

    spinlock_lock(&free_page_list_lock);
    if (list_empty(&free_page_list))
    {
        spinlock_unlock(&free_page_list_lock);
        return NULL;
    }
    fp = container_of(free_page_list.next, struct page, list);
    spinlock_unlock(&free_page_list_lock);
    
    list_del(&fp->list);
    list_add_tail(&fp->list, &used_page_list);
    return fp;
}

void free_page(struct page *fp)
{
    list_del(&fp->list);
    list_add_tail(&fp->list, &free_page_list);
}

void *get_physaddr(void *virtualaddr)
{
    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FFUL;

    unsigned long *pd = (unsigned long *)0xFFFFF000UL;
    // Here you need to check whether the PD entry is present.
    if ((pd[pdindex] & 0x1) == 0)
        return (void *)-1;

    unsigned long *pt = ((unsigned long *)0xFFC00000UL) + (0x400UL * pdindex);
    // Here you need to check whether the PT entry is present.
    if ((pt[ptindex] & 0x1) == 0)
        return (void *)-1;

    return (void *)((pt[ptindex] & ~0xFFFUL) + ((unsigned long)virtualaddr & 0xFFFUL));
}

int map_page(void *physaddr, void *virtualaddr, unsigned int flags)
{
    struct page *t;

    // Make sure that both addresses are page-aligned.
    unsigned long pdindex = (unsigned long)virtualaddr >> 22UL;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12UL & 0x03FFUL;

    unsigned long *pd = (unsigned long *)0xFFFFF000UL;
    // Here you need to check whether the PD entry is present.
    // When it is not present, you need to create a new empty PT and
    // adjust the PDE accordingly.
    // 如果对应页表不存在则分配一个页作为页表空间
    if ((pd[pdindex] & 0x1UL) == 0)
    {
        t = alloc_page();
        pd[pdindex] = (uint32_t)t->base | (flags & 0xFFFUL) | 0x01UL;
    }

    unsigned long *pt = ((unsigned long *)0xFFC00000UL) + (0x400UL * pdindex);
    // Here you need to check whether the PT entry is present.
    // When it is, then there is already a mapping present. What do you do now?
    if (pt[ptindex] & 0x1UL)
        return -1;

    physaddr = (void *)((uint32_t)physaddr & 0xfffff000UL);
    pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFFUL) | 0x01UL; // Present

    // Now you need to flush the entry in the TLB
    // or you might not notice the change.
    flush_tlb();
    return 0;
}

int unmap_page(void *virtualaddr)
{
    // Make sure that both addresses are page-aligned.
    unsigned long pdindex = (unsigned long)virtualaddr >> 22UL;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12UL & 0x03FFUL;

    unsigned long *pd = (unsigned long *)0xFFFFF000UL;
    unsigned long *pt = ((unsigned long *)0xFFC00000UL) + (0x400UL * pdindex);
    // Here you need to check whether the PD entry is present.
    // When it is not present, you need to create a new empty PT and
    // adjust the PDE accordingly.
    // 如果对应页表不存在则分配一个页作为页表空间
    if ((pd[pdindex] & 0x1UL) && (pt[ptindex] & 0x1UL))
    {
        pt[ptindex] &= ~0x1UL;
    }
    else
    {
        return -1;
    }

    // Now you need to flush the entry in the TLB
    // or you might not notice the change.
    flush_tlb();
    return 0;
}

int map_page_range(void *physaddr, void *virtualaddr, unsigned int flags, size_t len)
{
    int ret;
    uint32_t i;

    for (i = 0; i < len; i++)
    {
        ret = map_page((uint32_t *)((uint32_t)physaddr + (i * 0x1000)), (uint32_t *)((uint32_t)virtualaddr + (i * 0x1000)), flags);
        if (ret < 0)
            return ret;
    }
    ret = 0;
    return ret;
}

int unmap_page_range(void *virtualaddr, size_t len)
{
    int ret;
    uint32_t i;

    for (i = 0; i < len; i++)
    {
        ret = unmap_page((uint32_t *)((uint32_t)virtualaddr + (i * 0x1000)));
        if (ret < 0)
            return ret;
    }
    ret = 0;
    return ret;
}

void get_cr3(uint32_t *cr3)
{
    // asm volatile(
    //     "movl %%cr3, %0"
    //     : "=r"(*cr3)
    //     :
    //     : "memory");
}

void copy_kernel_pagedir(uint32_t *pd)
{
    uint32_t i;

    for (i = 768; i < 1024; i++)
    {
        pd[i] = kpage_directory[i];
    }
}