#include <gdt.h>
#include <int.h>
#include <lapic.h>
#include <printk.h>
#include <smp.h>
#include <task.h>
#include <x86.h>

void ap_idle_thread(void *args)
{
	while (1) {
		args = args;
		hlt();
	}
}

void ap_start(void)
{
	gdt_init();
	int_init();
	ap_lapic_init();

	run_thread(ap_idle_thread, (void *)(uint64_t)get_current_cpuid(),
	           "ap_idle_thread");
	while (1) {
		hlt();
	}
}