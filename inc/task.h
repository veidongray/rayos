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

extern struct task_list *current_runlist;
extern struct task_struct *current;
extern void switch_to_task(struct task_struct *);
extern void context_switch(struct task_struct *, struct task_struct *);
struct task_struct *kthread_create(void (*thread_func)(void *), void *arg, char *name);

#endif // TASK_H