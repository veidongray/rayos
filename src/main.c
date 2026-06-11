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
    uint64_t arg[6];
    arg[0] = SYS_OPEN;
    arg[1] = "/test";
    asm volatile(
        "movq %0, %%rdi\r\n"
        "int $0x80\r\n"
        :
        : "r"((uint64_t)arg));
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
