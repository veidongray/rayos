#include "task.h"
#include "idt.h"
#include <stddef.h>
#include "paging.h"
#include "print.h"
#include "gdt.h"
#include "libc/string.h"
#include "panic.h"
#include "libc/stdlib.h"
#include "mm.h"

INIT_TASK_CURRENT(current);
LIST_HEAD(task_list);

void task_init(void)
{
    // setup task_struct esp offset
    // task_esp from switch_task.S
    task_esp = offsetof(struct task_struct, esp);
}

static void task_exit(void)
{
    disable_irq();
    current->task_status = TASK_DEAD;
    scheduler();
    enable_irq();
}

struct task_struct *ktask_create(void (*task_func)(void *), void *arg, char *name)
{
    struct task_struct *ktask = kmalloc(sizeof(struct task_struct));
    uint32_t *stack = (uint32_t *)kmalloc(KTASK_STACK_LEN);
    uint32_t *stack_top;

    // make task stack
    memset(stack, 0x0, KTASK_STACK_LEN);
    stack_top = (uint32_t *)((uint32_t)stack + KTASK_STACK_LEN);
    *(--stack_top) = (uint32_t)arg;
    *(--stack_top) = (uint32_t)task_exit; // setup return address to thread_exit
    *(--stack_top) = (uint32_t)task_func;
    *(--stack_top) = 0x0; // eax
    *(--stack_top) = 0x0; // ecx
    *(--stack_top) = 0x0; // edx
    *(--stack_top) = 0x0; // ebx
    *(--stack_top) = 0x0; // ebp
    *(--stack_top) = 0x0; // esi
    *(--stack_top) = 0x0; // edi

    ktask->esp = (uint32_t)stack_top;
    ktask->stack = stack;
    ktask->tss_esp0 = 0;
    ktask->task_status = TASK_READY;
    ktask->task_level = TASK_KERN;
    strcpy(ktask->name, name);
    get_cr3(&ktask->page_dir);
    list_add(&ktask->list, &task_list);

    if (current == NULL)
    {
        // means first ktask
        current = ktask;
        current->task_status = TASK_RUNNING;
        switch_to(current);
    }
    return ktask;
}

void scheduler(void)
{
    struct task_struct *cur, *next;
    disable_irq();
    if (current != NULL)
    {
        switch (current->task_status)
        {
        case TASK_RUNNING:
            list_del(&current->list);
            list_add_tail(&current->list, &task_list);
            next = container_of(task_list.next, struct task_struct, list);

            cur = current;
            current = next;
            // update task status
            cur->task_status = TASK_READY;
            next->task_status = TASK_RUNNING;

            update_tss_esp0(next->tss_esp0);
            load_page_directory((uint32_t *)next->page_dir);
            context_switch(cur, next);
            break;

        case TASK_DEAD:
            list_del(&current->list);
            next = container_of(task_list.next, struct task_struct, list);

            cur = current;
            current = next;
            // update task status
            next->task_status = TASK_RUNNING;

            kfree(current->stack);
            kfree((uint32_t *)current->tss_esp0);
            kfree(current);

            update_tss_esp0(next->tss_esp0);
            load_page_directory((uint32_t *)next->page_dir);
            context_switch(cur, next);
            break;

        default:
            // Do nothing
            break;
        }
    }
    enable_irq();
}