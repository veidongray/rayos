#include <mm.h>
#include <task.h>

struct task_struct *current;
LIST_HEAD(task_list);

struct task_struct *task_create(void (*task_func)(void *), void *args, char *name, int flags)
{
    struct task_struct *task;

    task = (struct task_struct *)kmalloc(sizeof(struct task_struct));
    task->stack = (uint64_t *)kmalloc(TASK_STACK_SIZE_MAX);

    task->rsp = (uint64_t *)((uint64_t)task->stack + TASK_STACK_SIZE_MAX);

    *(--task->rsp) = 0x0;
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

    list_add(&task->list, &task_list);

    if (current == NULL)
    {
        current = task;
        task->status = TASK_RUNNING;
        switch_to(task->rsp);
    }
}