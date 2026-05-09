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

struct task_struct *utask_create(void (*task_func)(void *), void *arg, char *name)
{
    uint32_t i;
    uint32_t *stack, *stack_top, *task_code;
    uint32_t *user_pagedir, *user_table;
    struct task_struct *task;

    // copy kernel page dir
    user_pagedir = (uint32_t *)kmalloc_aligned(1024 * sizeof(uint32_t));
    memset(user_pagedir, 0, 1024 * sizeof(uint32_t));
    copy_kernel_pagedir(user_pagedir);

    // make user page table
    user_table = (uint32_t *)kmalloc_aligned(1024 * sizeof(uint32_t));
    memset(user_table, 0, 1024 * sizeof(uint32_t));
    for (i = 0; i < 1024; i++)
    {
        user_table[i] = ((uint32_t)alloc_page()->base & (~0xfffUL)) | 0x7UL;
    }

    // user task start at 0x40000000
    user_pagedir[256] = ((uint32_t)get_physaddr((uint32_t *)user_table) & (~0xfffUL)) | 0x7UL;

    // copy task
    task_code = (uint32_t *)0x40000000;
    if (map_page(user_table[0], task_code, 0x7) < 0)
    {
        PANIC("ERROR map\n");
    }
    memcpy(task_code, (uint8_t *)task_func, 2048);

    // make user stack
    stack = (uint32_t *)((uint32_t)task_code + 0x1000);
    stack_top = stack;
    *(--stack_top) = UDATA_SELECTOR;
    *(--stack_top) = (uint32_t)stack;
    *(--stack_top) = 0x202;
    *(--stack_top) = UCODE_SELECTOR;
    *(--stack_top) = task_code;
    *(--stack_top) = switch_to_user;
    *(--stack_top) = 0x0; // eax
    *(--stack_top) = 0x0; // ecx
    *(--stack_top) = 0x0; // edx
    *(--stack_top) = 0x0; // ebx
    *(--stack_top) = 0x0; // ebp
    *(--stack_top) = 0x0; // esi
    *(--stack_top) = 0x0; // edi

    task = (struct task_struct *)kmalloc_aligned(sizeof(struct task_struct));
    task->esp = (uint32_t)stack_top;
    task->stack = stack;
    task->tss_esp0 = (uint32_t)kmalloc_aligned(8192);
    task->task_status = TASK_READY;
    task->task_level = TASK_USER;
    task->page_dir = (uint32_t)get_physaddr(user_pagedir);
    strcpy(task->name, name);
    list_add(&task->list, &task_list);
    return task;
}

struct task_struct *ktask_create(void (*task_func)(void *), void *arg, char *name)
{
    uint32_t *stack;
    uint32_t *stack_top;
    struct task_struct *ktask;

    // make task stack
    stack = (uint32_t *)kmalloc_aligned(KTASK_STACK_LEN);
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

    ktask = (struct task_struct *)kmalloc_aligned(sizeof(struct task_struct));
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

size_t total_tasks(void)
{
    uint32_t count = 0;
    struct list_head *pos;
    list_for_each(pos, &task_list)
    {
        count++;
    }
    return count;
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