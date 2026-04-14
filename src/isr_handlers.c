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
extern struct task_struct task0, task1, task2;

void exception_handler(uint32_t vector)
{
    static uint32_t toggle = 0;
    pic_sendEOI(vector);
    if (vector == IRQ0_VECTOR)
    {
        set_systicks(get_systicks() + 1);
        // Need wait for 100 ticks to start the first context switch
        if (get_systicks() >= 100) {
            if (toggle == 0) {
                toggle = 1;
                struct task_struct *tmp = current_task;
                current_task = current_task->next;
                context_switch(tmp, tmp->next);
            }
            else if (toggle == 1) {
                toggle = 2;
                struct task_struct *tmp = current_task;
                current_task = current_task->next;
                context_switch(&task1, &task2);
            } else if (toggle == 2) {
                toggle = 0;
                struct task_struct *tmp = current_task;
                current_task = current_task->next;
                context_switch(&task2, &task0);
            }
        }
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
