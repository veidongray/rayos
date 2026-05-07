#include "task.h"
#include "kheap.h"
#include "idt.h"
#include <stddef.h>
#include "paging.h"
#include "print.h"
#include "gdt.h"
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
    uint32_t *stack = (uint32_t *)kmalloc(0x100000, KHEAP_ALLOC);
    uint32_t *stack_step;

    // make task stack
    memset(stack, 0x0, 0x100000);
    stack_step = (uint32_t)stack + 0x100000;
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
    strcpy(kthread->name, name);
    asm volatile(
        "movl %%cr3, %0"
        : "=r"(kthread->page_dir)
        :
        : "memory");
    kthread->task_level = TASK_KERN;
    kthread->task_scheduled = 0;
    kthread->tss_esp0 = (uint32_t)kmalloc(8192, KHEAP_ALLOC) + 8192;
    cga_printf("kthread esp0 0x%X\n", kthread->tss_esp0);

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
        update_tss_esp0(kthread->tss_esp0);
        load_page_directory(kthread->page_dir);
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

struct task_struct *utask_create(void *(task_func)(void *), void *arg, char *name)
{
    uint32_t i;
    struct task_struct *task = (struct task_struct *)kmalloc(sizeof(struct task_struct), KHEAP_ALLOC);

    // map vga address area for user task
    map_page((uint32_t *)0xb8000, (uint32_t *)0xb8000, 0x7);
    // make new pd
    uint32_t *new_pd = 0x40000000;
    map_page(alloc_page()->base, new_pd, 0x7);
    // copy vga map
    new_pd[0] = kpage_directory[0] | 0x7;
    for (i = 768; i < 1024; i++)
    {
        // copy kernel map
        new_pd[i] = kpage_directory[i];
    }
    new_pd[1023] = (uint32_t)get_physaddr(new_pd);

    // make new pt
    uint32_t *new_pt = 0x40001000;
    map_page(alloc_page()->base, new_pt, 0x7);
    for (i = 0; i < 1024; i++)
    {
        // alloc 4096 pages
        new_pt[i] = (uint32_t)alloc_page()->base | 0x7;
    }
    // from 0x80000000
    new_pd[512] = (uint32_t)get_physaddr(new_pt) | 0x7;

    // copy task code/data
    // user_ptr is user task start address
    uint32_t *user_ptr = 0x80000000;
    for (i = 0; i < 1024; i++)
    {
        map_page(new_pt[i], (uint32_t *)((uint32_t)user_ptr + (i * 0x1000)), 0x7);
    }
    memcpy(user_ptr, task_func, 4096);
    uint32_t *stack = (uint32_t *)((uint32_t)user_ptr + 0x400000);
    uint32_t *stack_top = stack;
    *(--stack_top) = UDATA_SELECTOR;
    *(--stack_top) = stack;
    asm volatile(
        "pushfl\n\t"
        "popl %0"
        : "=r"(*(--stack_top))
        :
        : "memory");
    // *(--stack_top) = 0x2;
    *(--stack_top) = UCODE_SELECTOR;
    *(--stack_top) = user_ptr;
    *(--stack_top) = 0x0; // eax
    *(--stack_top) = 0x0; // ecx
    *(--stack_top) = 0x0; // edx
    *(--stack_top) = 0x0; // ebx
    *(--stack_top) = 0x0; // ebp
    *(--stack_top) = 0x0; // esi
    *(--stack_top) = 0x0; // edi

    task->esp = stack_top;
    task->stack = stack;
    task->task_status = TASK_READY;
    strcpy(task->name, name);
    task->page_dir = (uint32_t)get_physaddr(new_pd);
    task->task_level = TASK_USER;
    task->task_scheduled = 0;
    task->tss_esp0 = (uint32_t)kmalloc(8192, KHEAP_ALLOC) + 8192;
    cga_printf("task esp0 0x%X\n", get_physaddr(task->tss_esp0));

    struct task_list *rl = (struct task_list *)kmalloc(sizeof(struct task_list), KHEAP_ALLOC);
    rl->task = task;
    rl->next = rl;
    rl->prev = rl;

    disable_irq();
    // switch to new user task
    rl->next = current_tasklist->next;
    rl->prev = current_tasklist;
    rl->next->prev = rl;
    current_tasklist->next = rl;
    enable_irq();

    return task;
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
        if (old_task->task_level == TASK_USER)
        {
            cga_printf("<==========>\n");
            cga_printf("U old_task esp 0x%x\n", old_task->esp);
            cga_printf("K new_task esp 0x%x\n", new_task->esp);
            // while (1) {
            //     asm volatile("cli;hlt;");
            // }
        }
        else
        {
            cga_printf("<==========>\n");
            cga_printf("K old_task esp 0x%x\n", old_task->esp);
            cga_printf("U new_task esp 0x%x\n", new_task->esp);
            // while (1) {
            //     asm volatile("cli;hlt;");
            // }
        }
        if ((new_task->task_scheduled == 0) && (new_task->task_level == TASK_USER))
        {
            cga_printf("USERUSERUSER\n");
            new_task->task_scheduled++;
            update_tss_esp0(new_task->tss_esp0);
            load_page_directory((uint32_t *)new_task->page_dir);
            switch_to_user(new_task);
        }
        update_tss_esp0(new_task->tss_esp0);
        load_page_directory((uint32_t *)new_task->page_dir);
        // context_switch(old_task, new_task);
    }
    enable_irq();
}