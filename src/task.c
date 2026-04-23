#include "task.h"
#include "kheap.h"
#include "idt.h"
#include <stddef.h>

struct task_list *current_runlist = 0;
struct task_struct *current;

struct task_struct *kthread_create(void (*thread_func)(void *), void *arg, char *name)
{
    uint32_t i;
    struct task_struct *kthread = kmalloc(sizeof(struct task_struct));
    uint32_t *stack = (uint32_t *)kmalloc(sizeof(uint32_t) * 2048);
    uint32_t *stack_step;

    // setup task_struct esp offset
    extern uint32_t task_esp;
    task_esp = offsetof(struct task_struct, esp);

    for (i = 0; i < 2048; i++)
        stack[i] = 0;

    // create stack
    stack_step = &stack[2048];
    *(--stack_step) = (uint32_t)arg;
    *(--stack_step) = (uint32_t)0x00000000; // unused fake's return address for thread_func
    *(--stack_step) = (uint32_t)thread_func;
    *(--stack_step) = 0x0; // eax
    *(--stack_step) = 0x0; // ecx
    *(--stack_step) = 0x0; // edx
    *(--stack_step) = 0x0; // ebx
    *(--stack_step) = 0x0; // ebp
    *(--stack_step) = 0x0; // esi
    *(--stack_step) = 0x0; // edi

    kthread->esp = (uint32_t)stack_step;
    kthread->stack = stack;
    for (i = 0; i < 32; i++)
        kthread->name[i] = name[i];

    struct task_list *rl = (struct task_list *)kmalloc(sizeof(struct task_list));
    rl->task = kthread;
    rl->next = rl;
    rl->prev = rl;

    disable_irq();
    if (current_runlist == 0)
    {
        // means first thread
        current_runlist = rl;
        current = current_runlist->task;
        first_task(kthread);
    }
    else
    {
        // add new task to runlist
        rl->next = current_runlist->next;
        rl->prev = current_runlist;
        rl->next->prev = rl;
        current_runlist->next = rl;
    }
    enable_irq();
    return kthread;
}