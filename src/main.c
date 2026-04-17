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

void second0_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second1_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second2_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second3_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second4_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second5_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second6_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second7_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second8_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second9_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second10_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second11_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second12_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second13_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second14_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}
void second15_init(void *arg){while (1) cga_printf("task: %s\n", current->name);}

void kernel_init(void *arg)
{
    kthread_create(second0_init, 0, "second0_init");
    kthread_create(second1_init, 0, "second1_init");
    kthread_create(second2_init, 0, "second2_init");
    kthread_create(second3_init, 0, "second3_init");
    kthread_create(second4_init, 0, "second4_init");
    kthread_create(second5_init, 0, "second5_init");
    kthread_create(second6_init, 0, "second6_init");
    kthread_create(second7_init, 0, "second7_init");
    kthread_create(second8_init, 0, "second8_init");
    kthread_create(second9_init, 0, "second9_init");
    kthread_create(second10_init, 0, "second10_init");
    kthread_create(second11_init, 0, "second11_init");
    kthread_create(second12_init, 0, "second12_init");
    kthread_create(second13_init, 0, "second13_init");
    kthread_create(second14_init, 0, "second14_init");
    kthread_create(second15_init, 0, "second15_init");
    kthread_create(second0_init, 0, "second0_init");
    kthread_create(second1_init, 0, "second1_init");
    kthread_create(second2_init, 0, "second2_init");
    kthread_create(second3_init, 0, "second3_init");
    kthread_create(second4_init, 0, "second4_init");
    kthread_create(second5_init, 0, "second5_init");
    kthread_create(second6_init, 0, "second6_init");
    kthread_create(second7_init, 0, "second7_init");
    kthread_create(second8_init, 0, "second8_init");
    kthread_create(second9_init, 0, "second9_init");
    kthread_create(second10_init, 0, "second10_init");
    kthread_create(second11_init, 0, "second11_init");
    kthread_create(second12_init, 0, "second12_init");
    kthread_create(second13_init, 0, "second13_init");
    kthread_create(second14_init, 0, "second14_init");
    kthread_create(second15_init, 0, "second15_init");
    kthread_create(second0_init, 0, "second0_init");
    kthread_create(second1_init, 0, "second1_init");
    kthread_create(second2_init, 0, "second2_init");
    kthread_create(second3_init, 0, "second3_init");
    kthread_create(second4_init, 0, "second4_init");
    kthread_create(second5_init, 0, "second5_init");
    kthread_create(second6_init, 0, "second6_init");
    kthread_create(second7_init, 0, "second7_init");
    kthread_create(second8_init, 0, "second8_init");
    kthread_create(second9_init, 0, "second9_init");
    kthread_create(second10_init, 0, "second10_init");
    kthread_create(second11_init, 0, "second11_init");
    kthread_create(second12_init, 0, "second12_init");
    kthread_create(second13_init, 0, "second13_init");
    kthread_create(second14_init, 0, "second14_init");
    kthread_create(second15_init, 0, "second15_init");
    kthread_create(second0_init, 0, "second0_init");
    kthread_create(second1_init, 0, "second1_init");
    kthread_create(second2_init, 0, "second2_init");
    kthread_create(second3_init, 0, "second3_init");
    kthread_create(second4_init, 0, "second4_init");
    kthread_create(second5_init, 0, "second5_init");
    kthread_create(second6_init, 0, "second6_init");
    kthread_create(second7_init, 0, "second7_init");
    kthread_create(second8_init, 0, "second8_init");
    kthread_create(second9_init, 0, "second9_init");
    kthread_create(second10_init, 0, "second10_init");
    kthread_create(second11_init, 0, "second11_init");
    kthread_create(second12_init, 0, "second12_init");
    kthread_create(second13_init, 0, "second13_init");
    kthread_create(second14_init, 0, "second14_init");
    kthread_create(second15_init, 0, "second15_init");
    while (1) asm volatile ("hlt\r\n");
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
    kthread_create(kernel_init, 0, "kernel_init");
    while (1) asm volatile("hlt\r\n");
}
