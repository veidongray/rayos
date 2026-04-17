#include "multiboot2.h"
#include "print.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "kheap.h"
#include "task.h"
#include <stdint.h>

extern uint32_t _mboot_info[];
extern uint32_t _mboot_magic[];
extern uint32_t host_total_mem;

void second3_init(void *arg)
{
    extern struct task_list *current;
    while (1) cga_printf("task: %s\n", current->task->name);
}

void second2_init(void *arg)
{
    kthread_create(second3_init, 0, "second3_init");
    extern struct task_list *current;
    while (1) cga_printf("task: %s\n", current->task->name);
}

void second1_init(void *arg)
{
    kthread_create(second2_init, 0, "second2_init");
    extern struct task_list *current;
    while (1) cga_printf("task: %s\n", current->task->name);
}

void second0_init(void *arg)
{
    kthread_create(second1_init, 0, "second1_init");
    extern struct task_list *current;
    while (1) cga_printf("task: %s\n", current->task->name);
}

void kernel_init(void *arg)
{
    kthread_create(second0_init, 0, "second0_init");
    extern struct task_list *current;
    while (1) cga_printf("task: %s\n", current->task->name);
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
    page_init();
    cga_init();
    kheap_init();
    struct task_struct *kinit;
    kinit = kthread_create(kernel_init, 0, "kernel_init");
    extern void switch_to_task(struct task_struct *);
    switch_to_task(kinit);
    while (1) asm volatile("hlt\r\n");
}
