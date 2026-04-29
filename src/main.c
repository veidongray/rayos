#include "multiboot2.h"
#include "print.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "kheap.h"
#include "task.h"
#include "pic_8259.h"
#include <stdint.h>
#include <stddef.h>
#include "libc/string.h"
#include "libc/stdlib.h"

extern uint32_t _mboot_info[];
extern uint32_t _mboot_magic[];
extern uint32_t host_total_mem;

void user_init(void)
{
    while (1)
    {
        *(volatile uint8_t *)0xb8000 = 'U';
        *(volatile uint8_t *)0xb8002 = 'S';
        *(volatile uint8_t *)0xb8004 = 'E';
        *(volatile uint8_t *)0xb8006 = 'R';
    }
}

struct task_struct *utask_create(void *(task_func)(void *), void *arg, char *name)
{
    uint32_t i;

    // map vga address area for user task
    map_page((uint32_t *)0xb8000, (uint32_t *)0xb8000, 0x7);
    // make new pd
    uint32_t *new_pd = 0x40000000;
    map_page(alloc_page()->base, new_pd, 0x7);
    // copy vga map
    new_pd[0] = kpage_directory[0];
    for (i = 768; i < 1024; i++)
    {
        // copy kernel map
        new_pd[i] = kpage_directory[i];
    }
    new_pd[1023] = (uint32_t)get_physaddr(new_pd) | 0x7;

    // make new pt
    uint32_t *new_pt = 0x40001000;
    map_page(alloc_page()->base, new_pt, 0x7);
    for (i = 0; i < 1024; i++)
    {
        // alloc 4096 pages
        new_pt[i] = (uint32_t)alloc_page()->base | 0x7;
    }
    // from 0x80000000
    new_pd[512] = (uint32_t)get_physaddr(new_pt) | 0x7;

    // copy task code/data
    // only copy 4K for test
    uint32_t *user_ptr = 0x80000000;
    map_page(new_pt[0], user_ptr, 0x7);
    memcpy(user_ptr, user_init, 4096);
    uint32_t *stack = user_ptr + 0x400;
    uint32_t *stack_top = stack;
    *(--stack_top) = UDATA_SELECTOR;
    *(--stack_top) = stack;
    *(--stack_top) = 0x2;
    *(--stack_top) = UCODE_SELECTOR;
    *(--stack_top) = user_ptr;

    load_page_directory((uint32_t *)get_physaddr(new_pd));

    asm volatile(
        "cli\r\n"
        "movl %0, %%esp\n\r" // 将 stack_top 加载到 esp
        "iret\n\r"           // 执行中断返回
        :
        : "r"(stack_top)
        : "memory");
}

void kernel_init(void *arg)
{
    arg = arg;

    utask_create(NULL, NULL, NULL);
    while (1)
    {
        cga_printf("kernel_init ... %s\n", current->name);
        asm volatile("hlt\r\n");
    }
}

void start_kernel(void)
{
    if (_mboot_magic[0] == 0x36d76289)
        parse_multiboot2_mmap((uint32_t *)_mboot_info[0]);
    else
    {
        // If we don't have a valid multiboot magic number, we can't trust the bootloader and should halt
        cga_printf("Lost Bootloader...\n");
        while (1)
            asm volatile("hlt\r\n");
    }

    // Check if we have at least 8MB of memory, otherwise we can't do much
    if (host_total_mem < 0x800000)
    {
        cga_printf("Not enough memory detected: %u bytes\n", host_total_mem);
        while (1)
            asm volatile("hlt\r\n");
    }
    gdt_init();
    idt_init();
    cga_init();
    page_init();
    kheap_init();

    // Never return
    kthread_create(kernel_init, 0, "kernel_init");
    while (1)
        asm volatile("hlt\r\n");
}
