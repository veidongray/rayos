#include <mm.h>
#include <x86.h>
#include <int.h>
#include <gdt.h>
#include <pic.h>
#include <page.h>
#include <task.h>
#include <uart.h>
#include <lapic.h>
#include <mutex.h>
#include <bitmap.h>
#include <multiboot2.h>
#include <lib/printf/printf.h>

static mutex_t mutex;

void user0(void *args)
{
    args = args;
    while (1)
        ;
}

void task1(void *args)
{
    args = args;
    mutex_lock(&mutex);
    printf("MUTEX %s\n", get_current()->name);
    mutex_unlock(&mutex);
    while (1)
    {
        printf("%s\n", get_current()->name);
    }
}

void task0(void *args)
{
    args = args;
    task_create(user0, 0, "user0", TASK_FLAGS_USER);
    mutex_init(&mutex);
    mutex_lock(&mutex);
    printf("MUTEX %s\n", get_current()->name);
    mutex_unlock(&mutex);
    task_create(task1, (void *)0x12344321, "task1", TASK_FLAGS_KERN);
    while (1)
    {
        printf("%s\n", get_current()->name);
    }
}

void start_kernel(void)
{
    total_memory_init();
    gdt_init();
    page_init();
    int_init();
    lapic_init();
    uart_init();
    task_manager_init();

    task_create(task0, (void *)0x12344321, "task0", TASK_FLAGS_KERN);
    while (1)
    {
        asm volatile("hlt");
    }
}
