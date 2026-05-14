#include "task.h"
#include "idt.h"
#include <stddef.h>
#include "paging.h"
#include "tty.h"
#include "gdt.h"
#include "libc/string.h"
#include "panic.h"
#include "libc/stdlib.h"
#include "mm.h"
#include "aligned.h"
#include "spinlock.h"
#include "list.h"

INIT_TASK_CURRENT(current);
LIST_HEAD(task_list);
SPINLOCK_INIT(task_list_lock);

void task_init(void)
{
    // setup task_struct esp offset
    // task_esp from switch_task.S
    task_esp = offsetof(struct task_struct, esp);
    spinlock_init(&task_list_lock);
}

static void task_exit(void)
{
    current->task_status = TASK_DEAD;
    scheduler();
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
    // total 4MB space from 0x40000000 to 0x40400000
    user_table = (uint32_t *)kmalloc_aligned(1024 * sizeof(uint32_t));
    memset(user_table, 0, 1024 * sizeof(uint32_t));
    for (i = 0; i < 1024; i++)
    {
        user_table[i] = ((uint32_t)alloc_page()->base & (~0xfffUL)) | 0x7UL;
    }

    // user task start at 0x40000000
    user_pagedir[256] = ((uint32_t)get_physaddr((uint32_t *)user_table) & (~0xfffUL)) | 0x7UL;
    user_pagedir[1023] = (uint32_t)get_physaddr(user_pagedir) | 0x7UL;

    // copy task
    // task code/data place in first 3MB
    task_code = (uint32_t *)TASK_CODE_BEGIN;
    if (map_page_range((uint32_t *)user_table[0], (uint32_t *)task_code, 0x7, 1024) < 0)
    {
        PANIC("MAP RANGE error\n");
    }
    memcpy(task_code, (uint8_t *)task_func, 3 * 1024 * 1024);

    // make user stack
    // task stack space place in last 1MB
    stack = (uint32_t *)((uint32_t)task_code + 0x300000);
    stack_top = (uint32_t *)((uint32_t)stack + 0x100000);
    *(--stack_top) = (uint32_t)arg;
    *(--stack_top) = (uint32_t)task_exit;
    *(--stack_top) = UDATA_SELECTOR;
    *(--stack_top) = (uint32_t)stack + 0x100000 - (2 * sizeof(uint32_t));
    *(--stack_top) = 0x202;
    *(--stack_top) = UCODE_SELECTOR;
    *(--stack_top) = (uint32_t)task_code;
    *(--stack_top) = (uint32_t)switch_to_user;
    *(--stack_top) = 0x0;            // eax
    *(--stack_top) = 0x0;            // ecx
    *(--stack_top) = 0x0;            // edx
    *(--stack_top) = 0x0;            // ebx
    *(--stack_top) = 0x0;            // ebp
    *(--stack_top) = 0x0;            // esi
    *(--stack_top) = 0x0;            // edi
    *(--stack_top) = UDATA_SELECTOR; // ds
    *(--stack_top) = UDATA_SELECTOR; // es
    *(--stack_top) = UDATA_SELECTOR; // fs
    *(--stack_top) = UDATA_SELECTOR; // gs

    // unmap task space
    if (unmap_page_range(task_code, 1024) < 0)
    {
        PANIC("UNMAP RANGE error\n");
    }

    task = (struct task_struct *)kmalloc_aligned(sizeof(struct task_struct));
    task->esp = (uint32_t)stack_top;
    task->stack = stack;
    task->tss_esp0 = (uint32_t)kmalloc_aligned(8192);
    task->task_status = TASK_READY;
    task->task_level = TASK_USER;
    task->page_dir = (uint32_t)get_physaddr(user_pagedir);
    strcpy(task->name, name);
    spinlock_lock(&task_list_lock);
    list_add(&task->list, &task_list);
    spinlock_unlock(&task_list_lock);
    return task;
}

struct task_struct *ktask_create(void (*task_func)(void *), void *arg, char *name)
{
    uint32_t *stack;
    uint32_t *stack_top;
    struct task_struct *ktask;
    
    // make task stack
    stack = (uint32_t *)kmalloc_aligned(TASK_STACK_LEN);
    memset(stack, 0x0, TASK_STACK_LEN);
    stack_top = (uint32_t *)((uint32_t)stack + TASK_STACK_LEN);
    *(--stack_top) = (uint32_t)arg;
    *(--stack_top) = (uint32_t)task_exit; // setup return address to thread_exit
    *(--stack_top) = (uint32_t)task_func;
    *(--stack_top) = 0x0;            // eax
    *(--stack_top) = 0x0;            // ecx
    *(--stack_top) = 0x0;            // edx
    *(--stack_top) = 0x0;            // ebx
    *(--stack_top) = 0x0;            // ebp
    *(--stack_top) = 0x0;            // esi
    *(--stack_top) = 0x0;            // edi
    *(--stack_top) = KDATA_SELECTOR; // ds
    *(--stack_top) = KDATA_SELECTOR; // es
    *(--stack_top) = KDATA_SELECTOR; // fs
    *(--stack_top) = KDATA_SELECTOR; // gs

    ktask = (struct task_struct *)kmalloc_aligned(sizeof(struct task_struct));
    ktask->esp = (uint32_t)stack_top;
    ktask->stack = stack;
    ktask->tss_esp0 = 0;
    ktask->task_status = TASK_READY;
    ktask->task_level = TASK_KERN;
    strcpy(ktask->name, name);
    get_cr3(&ktask->page_dir);
    
    spinlock_lock(&task_list_lock);
    list_add(&ktask->list, &task_list);
    spinlock_unlock(&task_list_lock);

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
    spinlock_lock(&task_list_lock);
    list_for_each(pos, &task_list)
    {
        count++;
    }
    spinlock_unlock(&task_list_lock);
    return count;
}

void scheduler(void)
{
    struct task_struct *cur, *next;
    
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
}