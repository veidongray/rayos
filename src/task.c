#include "task.h"
#include "kheap.h"
#include "idt.h"
#include <stddef.h>
#include "paging.h"
#include "libc/string.h"
#include "libc/stdlib.h"

struct task_list *current_tasklist = NULL;
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

    // make task stack
    memset(stack, 0x0, sizeof(uint32_t) * 2048);
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
    strcpy(kthread->name, "KERN");
    asm volatile(
        "movl %%cr3, %0"
        : "=r"(kthread->page_dir)
        :
        : "memory");

    struct task_list *rl = (struct task_list *)kmalloc(sizeof(struct task_list), KHEAP_ALLOC);
    rl->task = kthread;
    rl->next = rl;
    rl->prev = rl;

    disable_irq();
    if (current_tasklist == NULL)
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
    if (current_tasklist != NULL)
    {
        struct task_struct *old_task, *new_task;
        old_task = current_tasklist->task;
        new_task = current_tasklist->next->task;
        current_tasklist = current_tasklist->next;
        current = new_task;
        old_task->task_status = TASK_READY;
        new_task->task_status = TASK_RUNNING;
        // load_page_directory(current->page_dir);
        context_switch(old_task, new_task);
    }
    enable_irq();
}