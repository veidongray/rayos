#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "list.h"
#include "aligned.h"

// task status
#define TASK_RUNNING 0x0
#define TASK_READY 0x1
#define TASK_BLOCKED 0x2
#define TASK_DEAD 0x3

// task level
#define TASK_KERN 0x0
#define TASK_USER 0x1

#define TASK_STACK_LEN (128 * 1024)
#define TASK_CODE_BEGIN 0x40000000

struct task_struct
{
    char name[32];
    uint32_t esp;
    uint32_t *stack;
    uint32_t task_status;
    uint32_t page_dir;
    uint32_t task_level;
    uint32_t tss_esp0;
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