#include <stdint.h>
#include "print.h"
#include "pic.h"
#include "systicks.h"
struct task_struct
{
    uint32_t esp;
    uint32_t eip;
    uint32_t cr3;
    struct task_struct *next;
};

extern struct task_struct *current_task;
extern struct task_struct task0, task1;

void exception_handler(uint32_t vector)
{
    pic_sendEOI(vector);
    if (vector == IRQ0_VECTOR)
    {
        set_systicks(get_systicks() + 1);
    }
    else if (vector == 14)
    {
        cga_info("Page fault!\n");
        disable_irq();
        asm volatile("hlt\r\n");
    }
    else if (vector == IRQ1_VECTOR)
    {
        cga_info("Keyboard handle\n");
    }
    else
    {
        cga_info("Unhandled exception: %d\n", vector);
        disable_irq();
        asm volatile("hlt\r\n");
    }
}
