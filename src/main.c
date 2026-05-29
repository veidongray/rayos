#include <mm.h>
#include <x86.h>
#include <pci.h>
#include <int.h>
#include <gdt.h>
#include <pic.h>
#include <acpi.h>
#include <page.h>
#include <task.h>
#include <uart.h>
#include <lapic.h>
#include <mutex.h>
#include <bitmap.h>
#include <printk.h>
#include <multiboot2.h>

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
    printk("MUTEX %s\n", get_current()->name);
    mutex_unlock(&mutex);
    while (1)
    {
        printk("%s\n", get_current()->name);
    }
}

void task0(void *args)
{
    args = args;
    task_create(user0, 0, "user0", TASK_FLAGS_USER);
    mutex_init(&mutex);
    mutex_lock(&mutex);
    printk("MUTEX %s\n", get_current()->name);
    mutex_unlock(&mutex);
    task_create(task1, (void *)0x12344321, "task1", TASK_FLAGS_KERN);
    while (1)
    {
        printk("%s\n", get_current()->name);
    }
}

void start_kernel(void)
{
    total_memory_init();
    gdt_init();
    page_init();
    int_init();
    acpi_init();
    lapic_init();
    uart_init();
    pci_init();
    task_manager_init();

    extern bitmap_t page_alloc_bitmap;
    printk("total mem: %llu, used %llu, used percent %llu%%\n",
           get_total_mem(), bitmap_count_set(&page_alloc_bitmap) * 4096,
           bitmap_usage_percent(&page_alloc_bitmap));
    // task_create(task0, (void *)0x12344321, "task0", TASK_FLAGS_KERN);
    while (1)
    {
        asm volatile("hlt");
    }
}
