#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define TASK_RUNNING 0x0
#define TASK_READY 0x1
#define TASK_WAITING 0x2

struct task_struct
{
    uint32_t esp;
    uint32_t *stack;
    uint32_t task_status;
    char name[32];
    uint32_t page_dir;
};

struct task_list
{
    struct task_struct *task;
    struct task_list *prev;
    struct task_list *next;
};

extern struct task_list *current_tasklist;
extern struct task_struct *current;
extern void first_task(struct task_struct *);
extern void context_switch(struct task_struct *, struct task_struct *);

struct task_struct *kthread_create(void (*thread_func)(void *), void *arg, char *name);
void scheduler(void);

#endif // TASK_H