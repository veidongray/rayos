#include <acpi.h>
#include <bitmap.h>
#include <elf.h>
#include <ff.h>
#include <gdt.h>
#include <init.h>
#include <int.h>
#include <ioapic.h>
#include <lapic.h>
#include <mm.h>
#include <multiboot2.h>
#include <mutex.h>
#include <page.h>
#include <pci.h>
#include <pic.h>
#include <printk.h>
#include <string.h>
#include <sys/stat.h>
#include <task.h>
#include <uart.h>
#include <vfs.h>
#include <x86.h>

void kernel_init(void *args)
{
	char *buf;
	args = args;

	pr_info("/init running...");
	run_process("/init");

	buf = kzalloc(UART_BUF_SIZE);
	while (1) {
		// Do nothing.
		memset(buf, 0, UART_BUF_SIZE);
		uart_putc('>');
		uart_gets(buf);
		if (!strncmp(buf, "meminfo", 7)) {
			pr_info("total mem: %llu, used %llu, used percent "
			        "%llu%%",
			        get_total_mem(), memused(), memused_percent());
		}
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

	while (1) {
		asm volatile("hlt");
	}
}
