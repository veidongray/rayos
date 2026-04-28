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

void kernel_init(void *arg)
{
    arg = arg;
    while (1) {
        cga_printf("kernel_init ... %s\n", current->name);
        asm volatile ("hlt\r\n");
    }
}

void user_init(void)
{
    while (1) asm volatile ("cli\r\nhlt\r\n");
}

void start_kernel(void)
{
    if (_mboot_magic[0] == 0x36d76289)
        parse_multiboot2_mmap((uint32_t *)_mboot_info[0]);
    else
    {
        // If we don't have a valid multiboot magic number, we can't trust the bootloader and should halt
        cga_printf("Lost Bootloader...\n");
        while (1) asm volatile("hlt\r\n");
    }

    // Check if we have at least 8MB of memory, otherwise we can't do much
    if (host_total_mem < 0x800000)
    {
        cga_printf("Not enough memory detected: %u bytes\n", host_total_mem);
        while (1) asm volatile("hlt\r\n");
    }
    gdt_init();
    idt_init();
    cga_init();
    page_init();
    kheap_init();

    // user mode test
    uint32_t *new_pd = 0x200000;
    uint32_t *new_pt = 0x201000;
    struct page *test_pd = alloc_page();
    map_page(test_pd->base, (uint32_t *)new_pd, 0x7);
    struct page *test_pt = alloc_page();
    map_page(test_pt->base, (uint32_t *)new_pt, 0x7);

    for (int i = 0; i < 1024; i++) {
        new_pt[i] = (uint32_t)alloc_page()->base | 0x7;
    }
    extern uint32_t kpage_directory[1024] __attribute__((aligned(4096)));
    // from 0x40000000
    new_pd[256] = (uint32_t)test_pt->base | 0x7;
    for (int i = 0; i < 256; i++) {
        new_pd[768 + i] = ((uint32_t)kpage_directory[768 + i] & ~0xfffUL) | 0x7;
    }

    pic_disable();
    
    load_page_directory(test_pd->base);
    memcpy(0x40000000, user_init, 32);
    uint32_t *stack = 0x40000100;
    uint32_t *stack_top = stack;
    *(--stack_top) = UDATA_SELECTOR;
    *(--stack_top) = stack;
    *(--stack_top) = 0x202;
    *(--stack_top) = UCODE_SELECTOR;
    *(--stack_top) = 0x40000000;

    asm volatile (
        "cli\r\n"
        "movl %0, %%esp\n\r"   // 将 stack_top 加载到 esp
        "iret\n\r"             // 执行中断返回
        :
        : "r" (stack_top)
        : "memory"
    );

    // Never return
    // kthread_create(kernel_init, 0, "kernel_init");
    while (1) asm volatile("hlt\r\n");
}
