#include "x86.h"

static uint jiffies = 0x0;
static uint jiffies_interval = 0x0;
uint task_flag = 0;

void timer_handle(void)
{
    outb(0x20, 0x20);
    while (jiffies_interval++) {
        if (jiffies_interval == 9999999) {
            jiffies++;
            jiffies_interval = 0;
            task_flag = !task_flag;
            main();
        }
    }
}

void init_timer(void)
{
    uint divisor = 1193180 / 1000;   // 10ms

    outb(0x43, 0x36);
    outb(0x40, divisor & 0xff);
    outb(0x40, (divisor >> 8) & 0xff);
    set_8259a_irq_mask(0xfffc);
}