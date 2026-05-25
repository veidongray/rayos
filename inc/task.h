#ifndef TASK_H
#define TASK_H

#include <list.h>
#include <stdint.h>

#define TASK_FLAGS_USER (1 << 0)
#define TASK_FLAGS_KERN (1 << 1)

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
    uint64_t rsp0;
    uint64_t pml4;
    uint64_t *rsp;
    char name[32];
    uint64_t *stack;
    struct list_head list;
    enum task_status status;
};

struct context
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
} __attribute__((packed));

extern struct task_struct *current;

void task_exit(void);
void scheduler(void);
uint64_t get_cr3(void);
void set_cr3(uint64_t pml4addr);
extern void switch_to_user(void);                                    // from switch_to.S
extern void switch_to(uint64_t *rsp);                                // from switch_to.S
extern void context_switch(uint64_t **cur_rsp, uint64_t **next_rsp); // from switch_to.S
struct task_struct *task_create(void (*task_func)(void *), void *args, char *name, int flags);

#endif // TASK_H