#include <mm.h>
#include <x86.h>
#include <int.h>
#include <gdt.h>
#include <task.h>
#include <page.h>
#include <queue.h>
#include <lib/printf/printf.h>
#include <lib/string/string.h>

static struct task_struct *current = NULL;
static queue_t task_readyqueue;

static inline void task_manager_init(void)
{
    QUEUE_INIT(&task_readyqueue);
}

static inline int __kerntask_create(struct task_struct *task, void (*task_func)(void *), void *args)
{
    struct context *context;
    task->pml4 = read_cr3();
    task->stack = (uint64_t *)kzalloc(TASK_STACK_SIZE_MAX);
    if (task->stack == NULL)
        return NULL;

    task->rsp = (uint64_t *)((uint64_t)task->stack + TASK_STACK_SIZE_MAX);
    *(--task->rsp) = (uint64_t)task_exit;
    *(--task->rsp) = (uint64_t)task_func;

    task->rsp0 = (uint64_t)0x0;
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
    context->rflags = 0x202;
}

static inline int __usertask_create(struct task_struct *task, void (*task_func)(void *), void *args)
{
    uint64_t index;
    uint64_t pml4_idx, pdpt_idx, pd_idx, pt_idx;
    uint64_t *user_pml4 = (uint64_t *)0x0000700000000000;
    uint64_t *user_pdpt = (uint64_t *)0x0000700000001000;
    uint64_t *user_pd = (uint64_t *)0x0000700000002000;
    uint64_t *user_pt = (uint64_t *)0x0000700000003000;
    uint64_t *task_code = (uint64_t *)0x0000400000000000;
    struct context *context;

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

    task->rsp0 = (uint64_t)kzalloc(120 * 1024) + (120 * 1024);
    task->pml4 = get_physaddr((uint64_t)user_pml4);
    user_pml4[pml4_idx] = get_physaddr((uint64_t)user_pdpt) | 0x7;
    user_pdpt[pdpt_idx] = get_physaddr((uint64_t)user_pd) | 0x7;
    user_pd[pd_idx] = get_physaddr((uint64_t)user_pt) | 0x7;
    user_pt[pt_idx] = alloc_page() | 0x7;

    map_page(user_pt[0] & ~0xfff, task_code, 0x7);
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
    context->rflags = 0;

    // copy kernel pml4
    for (index = 256; index < 512; index++)
    {
        user_pml4[index] = ((volatile uint64_t *)(PML4_BASE << PAGE_SHIFT))[index];
    }
    unmap_page((uint64_t)user_pml4);
    unmap_page((uint64_t)user_pdpt);
    unmap_page((uint64_t)user_pd);
    unmap_page((uint64_t)user_pt);
    unmap_page((uint64_t)task_code);
    return 0;
}

struct task_struct *task_create(void (*task_func)(void *), void *args, char *name, int flags)
{
    struct context *context;
    struct task_struct *task;

    task = (struct task_struct *)kzalloc(sizeof(struct task_struct));
    if (task == NULL)
        return NULL;

    if (flags & TASK_FLAGS_KERN)
    {
        __kerntask_create(task, task_func, args);
    }
    else if (flags & TASK_FLAGS_USER)
    {
        __usertask_create(task, task_func, args);
    }

    task->flags = flags;
    task->status = TASK_READY;
    memset(task->name, 0x0, 32);
    memcpy(task->name, name, strlen(name));

    if (current == NULL)
    {
        current = task;
        task->status = TASK_RUNNING;
        switch_to(task->rsp);
    }

    queue_enqueue(&task_readyqueue, &task->list);

    return task;
}

void task_exit(void)
{
    local_irq_disable();
    current->status = TASK_EXIT;
    scheduler();
}

void scheduler(void)
{
    struct task_struct *old_task, *new_task;

    if (current)
    {
        if (!queue_empty(&task_readyqueue))
        {
            // 就绪队列不为空才进入任务切换
            old_task = current;
            new_task = container_of(queue_dequeue(&task_readyqueue), struct task_struct, list);

            new_task->status = TASK_RUNNING;
            if (old_task->status == TASK_RUNNING)
            {
                // 如果被切换任务是正常运行的TASK_RUNNIN状态才将其移到队列末尾
                old_task->status = TASK_READY;
                queue_enqueue(&task_readyqueue, &old_task->list);
            }

            current = new_task;
            write_cr3(new_task->pml4);
            update_tss_rsp0(new_task->rsp0);
            context_switch(&old_task->rsp, &new_task->rsp);
        }
    }
}

struct task_struct *get_current(void)
{
    return current;
}

queue_t *get_task_readyqueue(void)
{
    return &task_readyqueue;
}