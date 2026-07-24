#ifndef TASK_H
#define TASK_H

#include <list.h>
#include <queue.h>
#include <stdint.h>
#include <sys/stat.h>

#define TASK_FLAGS_USER (1 << 0)
#define TASK_FLAGS_KERN (1 << 1)

enum task_status {
	TASK_EXIT,
	TASK_DEAD,
	TASK_READY,
	TASK_RUNNING,
	TASK_BLOCKED
};

struct load_segment_address {
	uint64_t phys;
	uint64_t order;
	struct load_segment_address *next;
};

struct task_struct {
	int flags;
	uint64_t rsp0;
	uint64_t rsp0_base; // record rsp0 base for task exit
	uint64_t pml4;
	uint64_t *rsp;
	char name[32];

	uint64_t stack_basephys; // stack down physaddr for task exit
	uint64_t stack_order;    // stack space order for task exit
	uint64_t *stack;         // record stack base for task exit

	uint64_t user_pml4_phys;
	uint64_t user_pdpt_phys;
	uint64_t user_pd_phys;
	uint64_t user_pt_phys;

	uint64_t user_pml4_order;
	uint64_t user_pdpt_order;
	uint64_t user_pd_order;
	uint64_t user_pt_order;

	struct load_segment_address *ls_addr;

	struct list_head list;
	enum task_status status;
};

struct context {
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
	uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
	uint64_t rflags;
} __attribute__((packed));

/**
 * @brief iretq 栈帧结构（从低地址到高地址）
 *
 * 栈布局（高地址在上）：
 *   ┌─────────────┐  ← 高地址
 *   │     SS      │
 *   │     RSP     │
 *   │   RFLAGS    │
 *   │     CS      │
 *   │     RIP     │  ← RSP 指向此处
 *   └─────────────┘  ← 低地址
 */
struct iret_frame {
	uint64_t rip;    /* 返回指令地址 */
	uint64_t cs;     /* 代码段选择子 */
	uint64_t rflags; /* 标志寄存器 */
	uint64_t rsp;    /* 目标栈指针 */
	uint64_t ss;     /* 栈段选择子 */
} __attribute__((packed));

struct task_user_init_stack {
	struct context ctx;     // 任务上下文
	uint64_t iret_func;     // iret 跳板程序
	struct iret_frame iret; // iret 进入用户态栈帧
} __attribute__((packed));

typedef void (*thread_func_t)(void *);

void task_exit(void);
void scheduler(void);
void task_init(void);
extern void switch_to_user(void); // from switch_to.S
queue_t *get_task_readyqueue(void);
extern void switch_to(uint64_t *rsp); // from switch_to.S
struct task_struct *get_current(void);
extern void context_switch(uint64_t **cur_rsp,
                           uint64_t **next_rsp); // from switch_to.S
struct task_struct *run_thread(thread_func_t thread_func, void *args,
                               char *name);
int run_process(const char *pathname);
void kerntask_exit(int code);
void usertask_exit(int code);

#endif // TASK_H