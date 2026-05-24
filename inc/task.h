#ifndef TASK_H
#define TASK_H

#include <list.h>
#include <types.h>

#define TASK_USER (1 << 0)
#define TASK_KERN (1 << 1)

#define TASK_STACK_SIZE_MAX (128 * 1024)

enum task_status
{
    TASK_EXIT,
    TASK_DEAD,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED
};

struct task_struct
{
    int flags;
    uint64_t *rsp;
    char name[32];
    uint64_t *stack;
    struct list_head list;
    enum task_status status;
};

void task_exit(void);
void scheduler(void);
extern void switch_to(uint64_t *rsp); // from switch_to.S
extern void context_switch(uint64_t **cur_rsp, uint64_t **next_rsp); // from switch_to.S
struct task_struct *task_create(void (*task_func)(void *), void *args, char *name, int flags);

#endif // TASK_H