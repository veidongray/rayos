#include <mm.h>
#include <x86.h>
#include <int.h>
#include <gdt.h>
#include <pic.h>
#include <page.h>
#include <task.h>
#include <uart.h>
#include <lapic.h>
#include <printf.h>
#include <multiboot2.h>

void task1(void *args)
{
    args = args;
    printf("task1 %#x\n", args);
    while (1)
        ;
}

void task0(void *args)
{
    args = args;
    task_create(task1, (void *)0x12344321, "task1", TASK_KERN);
    printf("task0 %#x\n", args);
    while (1)
        ;
}

void start_kernel(void)
{
    total_memory_init();
    gdt_init();
    page_init();
    int_init();
    lapic_init();
    uart_init();

    task_create(task0, (void *)0x12344321, "task0", TASK_KERN);
    while (1)
    {
        asm volatile("hlt");
    }
}
