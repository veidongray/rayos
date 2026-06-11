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

void user0(void *args)
{
    args = args;
    while (1)
        ;
}

void kernel_init(void *args)
{
    args = args;
    task_create(vfs_task, NULL, "vfs_task", TASK_FLAGS_KERN);

    extern bitmap_t page_alloc_bitmap;
    printk("total mem: %llu, used %llu, used percent %llu%%\n",
           get_total_mem(), bitmap_count_set(&page_alloc_bitmap) * 4096,
           bitmap_usage_percent(&page_alloc_bitmap));

    task_create(user0, 0, "user0", TASK_FLAGS_USER);

    creat("/stdin", 0);
    open("/stdin", 0);
    creat("/stdout", 0);
    open("/stdout", 0);
    creat("/stderr", 0);
    open("/stderr", 0);

    creat("/test", 0);
    int fd = open("/test", 0);
    printk("fd = %d\n", fd);
    char *buf;

    buf = kmalloc(1024);
    write(fd, "wocaofulenimade", 16);
    read(fd, buf, 10);
    printk("%s\n", buf);
    kfree(buf);

    while (1)
    {
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
