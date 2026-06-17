#include <mm.h>
#include <x86.h>
#include <vfs.h>
#include <pci.h>
#include <int.h>
#include <elf.h>
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
#include <sys/stat.h>
#include <multiboot2.h>
#include <string.h>

void console_thread(void *args)
{
    char *buf;
    struct stat sb;

    args = args;

    while (1)
    {
        for (int i = 0; i < 0xffffff; i++)
            ;
        sys_stat("/stdout", &sb);
        if (sb.st_size)
        {
            buf = kzalloc(sb.st_size);
            memset(buf, 0, sb.st_size);
            sys_read(STDOUT, buf, sb.st_size);
            printk("%s\n", buf);
            kfree(buf);
        }
    }
}

void kernel_init(void *args)
{
    args = args;

    sys_create("/stdin");
    sys_open("/stdin");
    sys_create("/stdout");
    sys_open("/stdout");
    sys_create("/stderr");
    sys_open("/stderr");

    run_thread(console_thread, NULL, "console_thread");

    printk("/init running...\n");
    run_process("/init");

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

    run_thread(kernel_init, NULL, "kernel_init");
    while (1)
    {
        asm volatile("hlt");
    }
}
