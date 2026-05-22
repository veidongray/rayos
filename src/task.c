#include <mm.h>
#include <task.h>

struct task_struct *current;

struct task_struct *task_create(void *(task_func)(void *), void *args, char *name, int flags)
{
    struct task_struct *task;

    task = (struct task_struct *)kmalloc(sizeof(struct task_struct));
    kfree(task);
}