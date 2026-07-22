#include <lapic.h>
#include <task.h>
#include <timer.h>
#include <types.h>
#include <printk.h>

static volatile uint64_t jiffies = 0;

void lapic_timer_handler(void)
{
	lapic_send_eoi();
	jiffies++;
	scheduler();
}

uint64_t get_uptime_ms(void)
{
	return jiffies; // HZ=1000 → 1 jiffy = 1ms
}

void mdelay(uint32_t ms)
{
	uint64_t target = jiffies + ms;
	while (jiffies < target)
		asm volatile("hlt");
}

void sdelay(uint32_t s)
{
	uint64_t target = jiffies + (s * 1000);
	while (jiffies < target)
		asm volatile("hlt");
}