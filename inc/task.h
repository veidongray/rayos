#ifndef TASK_H
#define TASK_H

#include <list.h>
#include <types.h>

#define TASK_USER (1 << 0)
#define TASK_KERN (1 << 1)

struct task_struct
{
    int flags;
    char name[32];
    struct list_head list;
};

struct task_struct *task_create(void *(task_func)(void *), void *args, char *name, int flags);

#endif // TASK_H