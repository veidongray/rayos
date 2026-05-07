#include "task.h"
#include "kheap.h"
#include "idt.h"
#include <stddef.h>
#include "paging.h"
#include "print.h"
#include "gdt.h"
#include "libc/string.h"
#include "libc/stdlib.h"

struct task_struct *current;
LIST_HEAD(task_list);

void task_init(void)
{
    // setup task_struct esp offset
    // task_esp from switch_task.S
    extern uint32_t task_esp;
    task_esp = offsetof(struct task_struct, esp);
}

static void thread_exit(void)
{
    disable_irq();

    list_del(&current->list);

    kfree(current->stack);
    kfree(current->tss_esp0);
    kfree(current);
    enable_irq();

    scheduler();
}

struct task_struct *ktask_create(void (*thread_func)(void *), void *arg, char *name)
{
    uint32_t i;
    struct task_struct *ktask = kmalloc(sizeof(struct task_struct), KHEAP_ALLOC);
    uint32_t *stack = (uint32_t *)kmalloc(8192, KHEAP_ALLOC);
    uint32_t *stack_top;

    // make task stack
    memset(stack, 0x0, 8192);
    stack_top = (uint32_t)stack + 8192;
    *(--stack_top) = (uint32_t)arg;
    *(--stack_top) = (uint32_t)thread_exit; // setup return address to thread_exit
    *(--stack_top) = (uint32_t)thread_func;
    *(--stack_top) = 0x0; // eax
    *(--stack_top) = 0x0; // ecx
    *(--stack_top) = 0x0; // edx
    *(--stack_top) = 0x0; // ebx
    *(--stack_top) = 0x0; // ebp
    *(--stack_top) = 0x0; // esi
    *(--stack_top) = 0x0; // edi

    ktask->esp = (uint32_t)stack_top;
    ktask->stack = stack;
    ktask->task_status = TASK_READY;
    strcpy(ktask->name, name);
    get_cr3(&ktask->page_dir);
    ktask->task_level = TASK_KERN;
    ktask->task_scheduled = 0;
    ktask->tss_esp0 = (uint32_t)kmalloc(8192, KHEAP_ALLOC) + 8192;
    list_add(&ktask->list, &task_list);

    if (current == NULL)
    {
        // means first ktask
        current = ktask;
        switch_to(current);
    }
    return ktask;
}

struct task_struct *utask_create(void *(task_func)(void *), void *arg, char *name)
{
    uint32_t i;
    struct task_struct *task = NULL;

    return task;
}

void scheduler(void)
{
    disable_irq();
    if (current != NULL)
    {
        struct list_head *pos;
        struct task_struct *cur, *next;

        list_del(&current->list);
        list_add_tail(&current->list, &task_list);
        next = container_of(task_list.next, struct task_struct, list);

        cur = current;
        current = next;
        update_tss_esp0(next->tss_esp0);
        load_page_directory((uint32_t *)next->page_dir);
        context_switch(cur, next);
    }
    enable_irq();
}