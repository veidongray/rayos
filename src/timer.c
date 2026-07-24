#include <lapic.h>
#include <printk.h>
#include <smp.h>
#include <task.h>
#include <timer.h>
#include <types.h>
#include <x86.h>

static volatile uint64_t jiffies[MAX_CPUS];

void timer_init(void)
{
	for (int i = 0; i < MAX_CPUS; i++) {
		jiffies[i] = 0;
	}
}

void lapic_timer_handler(void)
{
	lapic_send_eoi();
	uint64_t cpuid = get_current_cpuid();
	jiffies[cpuid]++;
	scheduler();
}

uint64_t get_uptime_ms(void)
{
	uint64_t cpuid = get_current_cpuid();
	return jiffies[cpuid]; // HZ=1000 → 1 jiffy = 1ms
}

void mdelay(uint32_t ms)
{
	uint64_t cpuid = get_current_cpuid();
	uint64_t target = jiffies[cpuid] + ms;
	while (jiffies[cpuid] < target)
		hlt();
}

void sdelay(uint32_t s)
{
	uint64_t cpuid = get_current_cpuid();
	uint64_t target = jiffies[cpuid] + (s * 1000);
	while (jiffies[cpuid] < target)
		hlt();
}