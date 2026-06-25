#include <mm.h>
#include <ff.h>
#include <x86.h>
#include <vfs.h>
#include <pci.h>
#include <int.h>
#include <elf.h>
#include <gdt.h>
#include <pic.h>
#include <init.h>
#include <acpi.h>
#include <page.h>
#include <task.h>
#include <uart.h>
#include <lapic.h>
#include <mutex.h>
#include <bitmap.h>
#include <printk.h>
#include <ioapic.h>
#include <string.h>
#include <sys/stat.h>
#include <multiboot2.h>

void kernel_init(void *args)
{
    char *buf;
    args = args;

    printk("/init running...\n");
    run_process("/init");

    extern bitmap_t page_alloc_bitmap;
    printk("total mem: %llu, used %llu, used percent %llu%%\n",
           get_total_mem(), bitmap_count_set(&page_alloc_bitmap) * 4096,
           bitmap_usage_percent(&page_alloc_bitmap));

    buf = kzalloc(UART_BUF_SIZE);
    while (1)
    {
        // Do nothing.
        memset(buf, 0, UART_BUF_SIZE);
        uart_putc('>');
        uart_gets(buf);
    }
}

void start_kernel(void)
{
    total_memory_init();
    gdt_init();
    page_init();
    mm_init();
    int_init();
    uart_init();
    acpi_init();
    lapic_init();
    ioapic_init();
    pci_init();
    do_initcalls();
    vfs_init();
    task_init();

    run_thread(kernel_init, NULL, "kernel_init");

    while (1)
    {
        asm volatile("hlt");
    }
}
