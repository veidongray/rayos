#ifndef TASK_H
#define TASK_H

#include <stdint.h>

struct task_struct {
    uint32_t esp;
    char name[32];
};

struct task_list {
    struct task_struct *task;
    struct task_list *prev;
    struct task_list *next;
};

struct task_struct *kthread_create(void (*thread_func)(void *), void *arg, char *name);

#endif // TASK_H