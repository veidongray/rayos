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
    }
}

void kernel_init(void *arg)
{
    arg = arg;

    utask_create(user_init, NULL, "USER");
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
    // setup task_struct esp offset
    // task_esp from switch_task.S
    extern uint32_t task_esp;
    task_esp = offsetof(struct task_struct, esp);

    // Never return
    kthread_create(kernel_init, 0, "kernel_init");
    while (1)
        asm volatile("hlt\r\n");
}
