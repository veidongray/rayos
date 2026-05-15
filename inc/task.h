#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "list.h"
#include "aligned.h"

typedef enum
{
    TASK_RUNNING = 0,
    TASK_READY,
    TASK_BLOCKED,
    TASK_INTERRUPTIBLE,
    TASK_UNINTERRUPTIBLE,
    TASK_STOPPED,
    TASK_ZOMBIE,
    TASK_DEAD
} task_state_t;

typedef enum
{
    TASK_KERNEL = 0,
    TASK_USER = 1
} task_level_t;

#define TASK_STACK_LEN (128 * 1024)
#define TASK_CODE_BEGIN 0x40000000

struct task_struct
{
    char name[32];
    uint32_t esp;
    uint32_t *stack;
    uint32_t page_dir;
    uint32_t tss_esp0;
    task_level_t task_level;
    task_state_t task_status;
    struct list_head list;
};
#define INIT_TASK_CURRENT(cur) ALIGN_ATTR(4096) struct task_struct *(cur)

extern uint32_t task_esp;
extern INIT_TASK_CURRENT(current);
extern void switch_to(struct task_struct *);
extern void context_switch(struct task_struct *, struct task_struct *);
extern void switch_to_user(void);

struct task_struct *utask_create(void (*task_func)(void *), void *arg, char *name);
struct task_struct *ktask_create(void (*task_func)(void *), void *arg, char *name);
void scheduler(void);
void task_init(void);
size_t total_tasks(void);

#endif // TASK_H