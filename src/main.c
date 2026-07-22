#include <acpi.h>
#include <bitmap.h>
#include <elf.h>
#include <fcntl.h>
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
#include <smp.h>
#include <string.h>
#include <sys/stat.h>
#include <syscalls.h>
#include <task.h>
#include <timer.h>
#include <tsc.h>
#include <uart.h>
#include <vfs.h>
#include <x86.h>

void thread(void *args)
{
	while (1) {
		args = args;
		kerntask_exit(0);
	}
}

void kernel_init(void *args)
{
	char *buf;
	args = args;

	sys_creat("/stdin", O_SYNC);
	sys_creat("/stdout", O_SYNC);
	sys_creat("/stderr", O_SYNC);

	pr_info("/init running...");
	for (int i = 0; i < 64; i++) {
		run_process("/init");
		run_thread(thread, NULL, "thread");
	}

	buf = kzalloc(UART_BUF_SIZE);
	while (1) {
		// Do nothing.
		memset(buf, 0, UART_BUF_SIZE);
		uart_puts("# ");
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
	tsc_init();
	smp_init();
	pci_init();
	do_initcalls();
	vfs_init();
	task_init();

	run_thread(kernel_init, NULL, "kernel_init");

	while (1) {
		asm volatile("hlt");
	}
}
