#include "task.h"
#include "kheap.h"
#include "idt.h"

struct task_list *current = 0;

struct task_struct *kthread_create(void (*thread_func)(void *), void *arg, char *name)
{
    uint32_t i;
    struct task_struct *kthread = kmalloc(sizeof(struct task_struct));
    uint32_t *stack = (uint32_t *)kmalloc(sizeof(uint32_t) * 2048);

    for (i = 0; i < 2048; i++)
        stack[i] = 0;

    stack = &stack[2048];
    *(--stack) = (uint32_t)arg;
    *(--stack) = (uint32_t)thread_func;
    *(--stack) = 0x0; // eax
    *(--stack) = 0x0; // ecx
    *(--stack) = 0x0; // edx
    *(--stack) = 0x0; // ebx
    *(--stack) = 0x0; // ebp
    *(--stack) = 0x0; // esi
    *(--stack) = 0x0; // edi

    kthread->esp = (uint32_t)stack;
    for (i = 0; i < 32; i++)
        kthread->name[i] = name[i];

    struct task_list *rl = (struct task_list *)kmalloc(sizeof(struct task_list));
    rl->task = kthread;
    rl->next = rl;
    rl->prev = rl;

    disable_irq();
    if (current == 0)
    {
        // means first thread
        current = rl;
        extern void switch_to_task(struct task_struct *);
        switch_to_task(current->task);
    }
    else
    {
        rl->next = current->next;
        rl->prev = current;
        rl->next->prev = rl;
        current->next = rl;
    }
    enable_irq();
    return kthread;
}