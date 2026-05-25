#include <mm.h>
#include <int.h>
#include <gdt.h>
#include <task.h>
#include <page.h>
#include <lib/printf/printf.h>
#include <lib/string/string.h>

static struct task_struct *current = NULL;
LIST_HEAD(task_list);

__attribute__((aligned(4096))) static uint64_t user_rsp0[1024];

struct task_struct *task_create(void (*task_func)(void *), void *args, char *name, int flags)
{
    struct context *context;
    struct task_struct *task;

    task = (struct task_struct *)kzalloc(sizeof(struct task_struct));
    if (task == NULL)
        return NULL;

    // 构造任务栈
    if (flags & TASK_FLAGS_KERN)
    {
        task->pml4 = get_cr3();
        task->stack = (uint64_t *)kzalloc(TASK_STACK_SIZE_MAX);
        if (task->stack == NULL)
            return NULL;

        task->rsp = (uint64_t *)((uint64_t)task->stack + TASK_STACK_SIZE_MAX);
        *(--task->rsp) = (uint64_t)task_exit;
        *(--task->rsp) = (uint64_t)task_func;

        task->rsp0 = (uint64_t)user_rsp0;
        task->rsp = (uint64_t *)((uint64_t)task->rsp - sizeof(struct context));
        context = (struct context *)task->rsp;
        context->r15 = 0;
        context->r14 = 0;
        context->r13 = 0;
        context->r12 = 0;
        context->r11 = 0;
        context->r10 = 0;
        context->r9 = 0;
        context->r8 = 0;
        context->rax = 0;
        context->rbx = 0;
        context->rcx = 0;
        context->rdx = 0;
        context->rdi = (uint64_t)args;
        context->rsi = 0;
    }
    else if (flags & TASK_FLAGS_USER)
    {
        uint64_t index;
        uint64_t pml4_idx, pdpt_idx, pd_idx, pt_idx;
        uint64_t *user_pml4 = (uint64_t *)0x0000700000000000;
        uint64_t *user_pdpt = (uint64_t *)0x0000700000001000;
        uint64_t *user_pd = (uint64_t *)0x0000700000002000;
        uint64_t *user_pt = (uint64_t *)0x0000700000003000;
        uint64_t *task_code = (uint64_t *)0x0000400000000000;

        pml4_idx = ((uint64_t)task_code >> 39) & 0x1FF;
        pdpt_idx = ((uint64_t)task_code >> 30) & 0x1FF;
        pd_idx = ((uint64_t)task_code >> 21) & 0x1FF;
        pt_idx = ((uint64_t)task_code >> 12) & 0x1FF;

        // 因为kmalloc不能保证返回的地址4K对齐
        // 所以这里使用手动分配页再映射的方式确保对齐
        map_page(alloc_page(), (uint64_t)user_pml4, 0x7);
        map_page(alloc_page(), (uint64_t)user_pdpt, 0x7);
        map_page(alloc_page(), (uint64_t)user_pd, 0x7);
        map_page(alloc_page(), (uint64_t)user_pt, 0x7);

        task->rsp0 = (uint64_t)kzalloc(1024) + 1024;
        task->pml4 = get_physaddr((uint64_t)user_pml4);
        user_pml4[pml4_idx] = get_physaddr((uint64_t)user_pdpt) | 0x7;
        user_pdpt[pdpt_idx] = get_physaddr((uint64_t)user_pd) | 0x7;
        user_pd[pd_idx] = get_physaddr((uint64_t)user_pt) | 0x7;
        user_pt[pt_idx] = alloc_page() | 0x7;

        map_page(user_pt[0] & ~0xfff, 0x0000400000000000, 0x7);
        memset(task_code, 0, 0x1000);
        memcpy(task_code, task_func, 1024);
        task->rsp = (uint64_t *)((uint64_t)task_code + 2048);
        *(--task->rsp) = (uint64_t)task_exit;
        *(--task->rsp) = (uint64_t)UDATA_SELECTOR;
        *(--task->rsp) = (uint64_t)((uint64_t)task_code + 2040);
        *(--task->rsp) = (uint64_t)0x202;
        *(--task->rsp) = (uint64_t)UCODE_SELECTOR;
        *(--task->rsp) = (uint64_t)task_code;
        *(--task->rsp) = (uint64_t)switch_to_user;

        task->rsp = (uint64_t *)((uint64_t)task->rsp - sizeof(struct context));
        context = (struct context *)task->rsp;
        context->r15 = 0;
        context->r14 = 0;
        context->r13 = 0;
        context->r12 = 0;
        context->r11 = 0;
        context->r10 = 0;
        context->r9 = 0;
        context->r8 = 0;
        context->rax = 0;
        context->rbx = 0;
        context->rcx = 0;
        context->rdx = 0;
        context->rdi = (uint64_t)args;
        context->rsi = 0;

        // copy kernel pml4
        for (index = 256; index < 512; index++)
        {
            user_pml4[index] = ((volatile uint64_t *)(PML4_BASE << PAGE_SHIFT))[index];
        }
        unmap_page((uint64_t)user_pml4);
        unmap_page((uint64_t)user_pdpt);
        unmap_page((uint64_t)user_pd);
        unmap_page((uint64_t)user_pt);
    }

    task->flags = flags;
    task->status = TASK_READY;
    memset(task->name, 0x0, 32);
    memcpy(task->name, name, strlen(name));
    list_add(&task->list, &task_list);

    if (current == NULL)
    {
        current = task;
        task->status = TASK_RUNNING;
        switch_to(task->rsp);
    }

    return task;
}

void task_exit(void)
{
    disable_irq();
    current->status = TASK_EXIT;
    scheduler();
}

void scheduler(void)
{
    struct task_struct *cur, *next;

    if (current)
    {
        switch (current->status)
        {
        case TASK_RUNNING:
            list_del(&current->list);
            list_add_tail(&current->list, &task_list);
            next = container_of(task_list.next, struct task_struct, list);
            while (next->status == TASK_DEAD)
            {
                list_del(&next->list);
                kfree(next->stack);
                kfree(next);
                next = container_of(task_list.next, struct task_struct, list);
            }

            cur = current;
            current = next;
            // update task status
            cur->status = TASK_READY;
            next->status = TASK_RUNNING;

            set_cr3(next->pml4);
            update_tss_rsp0(next->rsp0);
            context_switch(&cur->rsp, &next->rsp);
            break;

        case TASK_EXIT:
            list_del(&current->list);
            list_add_tail(&current->list, &task_list);
            next = container_of(task_list.next, struct task_struct, list);

            cur = current;
            current = next;
            // update task status
            cur->status = TASK_DEAD;
            next->status = TASK_RUNNING;

            set_cr3(next->pml4);
            update_tss_rsp0(next->rsp0);
            context_switch(&cur->rsp, &next->rsp);
            break;

        default:
            break;
        }
    }
}

void set_cr3(uint64_t pml4addr)
{
    asm volatile(
        "movq %0, %%rax\r\n"
        "movq %%rax, %%cr3\r\n"
        :
        : "r"(pml4addr)
        : "rax");
}

uint64_t get_cr3(void)
{
    uint64_t retval;

    asm volatile(
        "movq %%cr3, %0"
        : "=r"(retval)
        :
        : "rax");
    return retval;
}

struct task_struct *get_current(void)
{
    return current;
}