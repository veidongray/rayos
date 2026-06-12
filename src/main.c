#include <mm.h>
#include <x86.h>
#include <vfs.h>
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
#include <syscall.h>
#include <multiboot2.h>

void kernel_init(void *args)
{
    int fd;
    char *data;
    args = args;

    creat("/stdin", 0);
    open("/stdin", 0);
    creat("/stdout", 0);
    open("/stdout", 0);
    creat("/stderr", 0);
    open("/stderr", 0);

    fd = open("/init", 0);
    if (fd > 0)
    {
        data = kzalloc(1024);
        read(fd, data, 16);
        printk("/init running...\n");
        task_create(data, 0, "init", TASK_FLAGS_USER);
        kfree(data);
    }

    extern bitmap_t page_alloc_bitmap;
    printk("total mem: %llu, used %llu, used percent %llu%%\n",
           get_total_mem(), bitmap_count_set(&page_alloc_bitmap) * 4096,
           bitmap_usage_percent(&page_alloc_bitmap));

    while (1)
    {
        // Do nothing.
    }
}

void start_kernel(void)
{
    total_memory_init();
    gdt_init();
    page_init();
    int_init();
    uart_init();
    acpi_init();
    lapic_init();
    pci_init();
    task_manager_init();

    task_create(kernel_init, NULL, "kernel_init", TASK_FLAGS_KERN);
    while (1)
    {
        asm volatile("hlt");
    }
}
