#include "task.h"
#include "kheap.h"
#include "idt.h"
#include <stddef.h>

struct task_list *current_tasklist = 0;
struct task_struct *current;

static void thread_exit(void)
{
    struct task_list *prev, *next, *ptr;

    disable_irq();

    ptr = current_tasklist;
    prev = ptr->prev;
    next = ptr->next;
    prev->next = next;
    next->prev = prev;

    kfree(current_tasklist);
    kfree(current->stack);
    kfree(current);

    scheduler();
    enable_irq();
}

struct task_struct *kthread_create(void (*thread_func)(void *), void *arg, char *name)
{
    uint32_t i;
    struct task_struct *kthread = kmalloc(sizeof(struct task_struct), KHEAP_ALLOC);
    uint32_t *stack = (uint32_t *)kmalloc(sizeof(uint32_t) * 2048, KHEAP_ALLOC);
    uint32_t *stack_step;

    // setup task_struct esp offset
    // task_esp from switch_task.S
    extern uint32_t task_esp;
    task_esp = offsetof(struct task_struct, esp);

    for (i = 0; i < 2048; i++)
        stack[i] = 0;

    // make task stack
    stack_step = &stack[2048];
    *(--stack_step) = (uint32_t)arg;
    *(--stack_step) = (uint32_t)thread_exit; // setup return address to thread_exit
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
    kthread->task_status = TASK_READY;
    for (i = 0; i < 32; i++)
        kthread->name[i] = name[i];

    struct task_list *rl = (struct task_list *)kmalloc(sizeof(struct task_list), KHEAP_ALLOC);
    rl->task = kthread;
    rl->next = rl;
    rl->prev = rl;

    disable_irq();
    if (current_tasklist == 0)
    {
        // means first thread
        current_tasklist = rl;
        current = current_tasklist->task;
        current->task_status = TASK_RUNNING;
        first_task(kthread);
    }
    else
    {
        // add new task to tasklist
        rl->next = current_tasklist->next;
        rl->prev = current_tasklist;
        rl->next->prev = rl;
        current_tasklist->next = rl;
    }
    enable_irq();
    return kthread;
}

void scheduler(void)
{
    disable_irq();
    if (current_tasklist != 0)
    {
        struct task_struct *old_task, *new_task;
        old_task = current_tasklist->task;
        new_task = current_tasklist->next->task;
        current_tasklist = current_tasklist->next;
        current = new_task;
        old_task->task_status = TASK_READY;
        new_task->task_status = TASK_RUNNING;
        context_switch(old_task, new_task);
    }
    enable_irq();
}