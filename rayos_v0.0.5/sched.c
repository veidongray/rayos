#include "x86.h"

struct list_head {
    struct list_head *next;
};

struct task_struct {
    struct tss *tss;
};

struct tss {
    uint eax;
    uint ebx;
};

extern uint task_flag;

void schedule(void)
{
    if (!task_flag) {
        sti();
        task0();
    } else {
        sti();
        task1();
    }
}