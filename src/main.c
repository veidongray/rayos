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
		// 运行一段时间然后退出
		sdelay(10);
		kerntask_exit(0);
	}
}

void kernel_init(void *args)
{
	char *buf;
	char name[32];
	args = args;

	pr_info("/init running...");
	run_process("/init");

	run_thread(thread, NULL, name);

	uint64_t up_time = get_uptime_ms();
	pr_info("Up time %llum%llu.%llus", up_time / 1000 / 60,
	        (up_time / 1000) % 60, up_time % 1000);

	buf = kzalloc(UART_BUF_SIZE);
	while (1) {
		memset(buf, 0, UART_BUF_SIZE);
		uart_puts("# ");
		uart_gets(buf);
		if (!strncmp(buf, "meminfo", 7)) {
			pr_info("total mem: %llu, used %llu, used percent "
			        "%llu%%",
			        get_total_mem(), memused(), memused_percent());
		}
		hlt();
	}
}

void start_kernel(void)
{
	total_memory_init();
	gdt_init();
	page_init();
	mm_init();
	int_init();
	pic_remap(0x20, 0x28);
	pic_timer_init();
	uart_init();
	acpi_init();
	lapic_init();
	ioapic_init();
	tsc_init();
	pci_init();
	do_initcalls();
	vfs_init();
	task_init();
	smp_init();

	run_thread(kernel_init, NULL, "kernel_init");

	while (1) {
		hlt();
	}
}
