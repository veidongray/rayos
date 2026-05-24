#include <mm.h>
#include <int.h>
#include <task.h>

struct task_struct *current = NULL;
LIST_HEAD(task_list);

struct task_struct *task_create(void (*task_func)(void *), void *args, char *name, int flags)
{
    struct task_struct *task;

    task = (struct task_struct *)kmalloc(sizeof(struct task_struct));
    if (task == NULL)
        return NULL;
    task->stack = (uint64_t *)kmalloc(TASK_STACK_SIZE_MAX);
    if (task->stack == NULL)
        return NULL;

    task->rsp = (uint64_t *)((uint64_t)task->stack + TASK_STACK_SIZE_MAX);

    // 构造任务栈
    *(--task->rsp) = (uint64_t)task_exit;
    *(--task->rsp) = (uint64_t)task_func;
    *(--task->rsp) = 0x0;
    *(--task->rsp) = 0x0;
    *(--task->rsp) = 0x0;
    *(--task->rsp) = 0x0;
    *(--task->rsp) = 0x0;
    *(--task->rsp) = (uint64_t)args; // %rdi
    *(--task->rsp) = 0x0;
    *(--task->rsp) = 0x0;
    *(--task->rsp) = 0x0;
    *(--task->rsp) = 0x0;
    *(--task->rsp) = 0x0;
    *(--task->rsp) = 0x0;
    *(--task->rsp) = 0x0;
    *(--task->rsp) = 0x0;
    *(--task->rsp) = 0x0;

    task->flags = flags;
    task->status = TASK_READY;
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

            context_switch(&cur->rsp, &next->rsp);
            break;

        default:
            break;
        }
    }
}