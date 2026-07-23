#include <algo.h>
#include <align.h>
#include <elf.h>
#include <ff.h>
#include <gdt.h>
#include <int.h>
#include <mm.h>
#include <page.h>
#include <printk.h>
#include <queue.h>
#include <smp.h>
#include <string.h>
#include <syscalls.h>
#include <task.h>
#include <vfs.h>
#include <x86.h>

#define KRSP0_SIZE (4 * 1024)
#define KSTACK_SIZE (16 * 1024)

#define URSP0_SIZE (16 * 1024)
#define USTACK_SIZE (512 * 1024)

#define USER_SPACE_STACK_PAGES(stack_len) (uint64_t)((stack_len) / PAGE_SIZE)
#define USER_SPACE_PML4_BASE 0x0000700000000000ULL
#define USER_SPACE_PDPT_BASE 0x0000700000001000ULL
#define USER_SPACE_PD_BASE 0x0000700000002000ULL
#define USER_SPACE_PT_BASE 0x0000700000003000ULL

static struct task_struct *current[MAX_CPUS];
static queue_t task_readyqueue[MAX_CPUS];
static queue_t task_exitqueue[MAX_CPUS];
static spinlock_t run_process_spinlock;
static spinlock_t run_thread_spinlock;

void task_init(void)
{
	for (int i = 0; i < MAX_CPUS; i++) {
		current[i] = NULL;
		QUEUE_INIT(&task_readyqueue[i]);
		QUEUE_INIT(&task_exitqueue[i]);
	}
	spinlock_init(&run_process_spinlock);
	spinlock_init(&run_thread_spinlock);
}

/* 将任务放入当前负载最低的就绪队列 */
static inline void sched_enqueue_balanced(struct task_struct *task)
{
	int loadlist[MAX_CPUS];
	for (int i = 0; i < MAX_CPUS; i++)
		loadlist[i] = task_readyqueue[i].nr_list;

	int idx = find_extreme_index(loadlist, MAX_CPUS, min_cmp);
	uint32_t *ap_ids = get_ap_ids();

	queue_enqueue(&task_readyqueue[ap_ids[idx]], &task->list);
}

/**
 * @brief 计算映射 nr_pages 个 4KB 物理页需要每一级页表的项数
 * 		  返回每一级需要分配的页表项数量
 *
 * @param nr_pages 需要的页数
 * @param pml4_count
 * @param pdpt_count
 * @param pd_count
 * @param pt_count
 */
static inline void calc_page_table_counts(uint64_t nr_pages,
                                          uint64_t *pml4_count,
                                          uint64_t *pdpt_count,
                                          uint64_t *pd_count,
                                          uint64_t *pt_count)
{
	if (nr_pages == 0) {
		*pml4_count = *pdpt_count = *pd_count = *pt_count = 0;
		return;
	}

	// 1. 最底层的 PT (Page Table)：每个 PT 项映射 4KB
	*pt_count = nr_pages; // 需要 nr_pages 个 PT 项

	// 2. PD (Page Directory)：每个 PD 项映射 2MB (512 * 4KB)
	*pd_count = (*pt_count + 511) / 512; // 向上对齐

	// 3. PDPT (Page Directory Pointer Table)：每个 PDPT 项映射 1GB (512 *
	// 2MB)
	*pdpt_count = (*pd_count + 511) / 512;

	// 4. PML4 (Page Map Level 4)：每个 PML4 项映射 512GB (512 * 1GB)
	*pml4_count = (*pdpt_count + 511) / 512;
}

static inline int __kerntask_create(struct task_struct *task,
                                    thread_func_t thread_func, void *args)
{
	task->pml4 = read_cr3();
	task->stack = (uint64_t *)kzalloc(KSTACK_SIZE);
	if (task->stack == NULL)
		return -1;

	task->rsp0_base = (uint64_t)kzalloc(KRSP0_SIZE);
	task->rsp0 = task->rsp0_base + KRSP0_SIZE;

	task->rsp = (uint64_t *)((uint64_t)task->stack + KSTACK_SIZE);
	*(--task->rsp) = (uint64_t)thread_func;

	task->rsp = (uint64_t *)((uint64_t)task->rsp - sizeof(struct context));
	struct context *context = (struct context *)task->rsp;
	memset(context, 0, sizeof(struct context));
	context->rdi = (uint64_t)args;
	context->rflags = 0x202;

	return 0;
}

void kerntask_exit(int code)
{
	code = code;
	struct task_struct *task = get_current();

	task->status = TASK_EXIT;

	local_irq_disable();
	scheduler();
}

void usertask_exit(int code)
{
	code = code;
	struct task_struct *task;

	task = get_current();
	task->status = TASK_EXIT;

	// It deson't call irq_disable() because it from syscall, which is auto
	// disable IRQ
	scheduler();
}

/**
 * @brief 为新的用户任务创建上下文和内存映射
 *
 * @param task
 * @param last_vaddr 最后一个LOAD段的地址
 * @param last_memsz 最后一个LOAD段的大小
 * @param start 第一个LOAD段的起始虚拟地址
 * @param entry 程序入口地址
 * @param nr_pages 程序映射的页数量
 * @param args
 * @return int
 */
static inline int __usertask_create(struct task_struct *task, uint64_t end,
                                    void *start, void *entry, void *args)
{
	uint64_t *user_pml4 = (uint64_t *)USER_SPACE_PML4_BASE;
	uint64_t *user_pdpt = (uint64_t *)USER_SPACE_PDPT_BASE;
	uint64_t *user_pd = (uint64_t *)USER_SPACE_PD_BASE;
	uint64_t *user_pt = (uint64_t *)USER_SPACE_PT_BASE;

	// 计算页表索引
	uint64_t pml4_idx, pdpt_idx, pd_idx, pt_idx;
	pml4_idx = ((uint64_t)start >> 39) & 0x1FF;
	pdpt_idx = ((uint64_t)start >> 30) & 0x1FF;
	pd_idx = ((uint64_t)start >> 21) & 0x1FF;
	pt_idx = ((uint64_t)start >> 12) & 0x1FF;

	// 计算每一级页表分别需要多少个项
	uint64_t pml4_count, pdpt_count, pd_count, pt_count;
	size_t nr_pages = (end - (uint64_t)start) >> PAGE_SHIFT;
	calc_page_table_counts(nr_pages, &pml4_count, &pdpt_count, &pd_count,
	                       &pt_count);

	// 因为kmalloc不能保证返回的地址4K对齐
	// 所以这里使用手动分配页再映射的方式确保对齐
	uint64_t user_pml4_order = size_to_order(pml4_count * sizeof(uint64_t));
	uint64_t user_pml4_phys = alloc_pages(user_pml4_order);
	task->user_pml4_order = user_pml4_order;
	task->user_pml4_phys = user_pml4_phys;
	map_page_range(user_pml4_phys, (uint64_t)user_pml4, MAP_USER_RW,
	               pml4_count);

	uint64_t user_pdpt_order = size_to_order(pdpt_count * sizeof(uint64_t));
	uint64_t user_pdpt_phys = alloc_pages(user_pdpt_order);
	task->user_pdpt_order = user_pdpt_order;
	task->user_pdpt_phys = user_pdpt_phys;
	map_page_range(user_pdpt_phys, (uint64_t)user_pdpt, MAP_USER_RW,
	               pdpt_count);

	uint64_t user_pd_order = size_to_order(pd_count * sizeof(uint64_t));
	uint64_t user_pd_phys = alloc_pages(user_pd_order);
	task->user_pd_order = user_pd_order;
	task->user_pd_phys = user_pd_phys;
	map_page_range(user_pd_phys, (uint64_t)user_pd, MAP_USER_RW, pd_count);

	uint64_t user_pt_order = size_to_order(pt_count * sizeof(uint64_t));
	uint64_t user_pt_phys = alloc_pages(user_pt_order);
	task->user_pt_order = user_pt_order;
	task->user_pt_phys = user_pt_phys;
	map_page_range(user_pt_phys, (uint64_t)user_pt, MAP_USER_RW, pt_count);

	// 配置页表
	task->pml4 = user_pml4_phys;

	// 复制内核上半部页表
	for (int index = 256; index < 511; index++) {
		user_pml4[index] =
		        ((volatile uint64_t *)(PML4_BASE << PAGE_SHIFT))[index];
	}
	user_pml4[511] = ((uint64_t)user_pml4_phys) | MAP_USER_RW;

	// 填充页表
	for (size_t i = 0; i < pml4_count; i++) {
		user_pml4[pml4_idx + i] =
		        ((uint64_t)user_pdpt_phys + (i * PAGE_SIZE)) |
		        MAP_USER_RW;
	}
	for (size_t i = 0; i < pdpt_count; i++) {
		user_pdpt[pdpt_idx + i] =
		        ((uint64_t)user_pd_phys + (i * PAGE_SIZE)) |
		        MAP_USER_RW;
	}
	for (size_t i = 0; i < pd_count; i++) {
		user_pd[pd_idx + i] =
		        ((uint64_t)user_pt_phys + (i * PAGE_SIZE)) |
		        MAP_USER_RW;
	}
	for (size_t i = 0; i < pt_count; i++) {
		user_pt[pt_idx + i] =
		        get_physaddr((uint64_t)start + (i * PAGE_SIZE)) |
		        MAP_USER_RW;
	}

	// 分配 tss rsp0
	task->rsp0_base = (uint64_t)kzalloc(URSP0_SIZE);
	task->rsp0 = task->rsp0_base + URSP0_SIZE;

	// 初始化栈
	task->stack = (uint64_t *)end;
	task->rsp = (uint64_t *)((uint64_t)task->stack -
	                         sizeof(struct task_user_init_stack));
	struct task_user_init_stack *init_stack =
	        (struct task_user_init_stack *)task->rsp;
	init_stack->iret.ss = (uint64_t)UDATA_SELECTOR;
	init_stack->iret.rsp = (uint64_t)end;
	init_stack->iret.rflags = (uint64_t)0x202;
	init_stack->iret.cs = (uint64_t)UCODE_SELECTOR;
	init_stack->iret.rip = (uint64_t)entry;
	init_stack->iret_func = (uint64_t)switch_to_user;
	init_stack->ctx.rdi = (uint64_t)args;

	// 卸载临时映射
	unmap_page_range((uint64_t)user_pml4, pml4_count);
	unmap_page_range((uint64_t)user_pdpt, pdpt_count);
	unmap_page_range((uint64_t)user_pd, pd_count);
	unmap_page_range((uint64_t)user_pt, pt_count);
	return 0;
}

struct task_struct *run_thread(thread_func_t thread_func, void *args,
                               char *name)
{
	struct task_struct *task;

	local_irq_disable();
	spinlock_lock(&run_thread_spinlock);

	task = (struct task_struct *)kzalloc(sizeof(struct task_struct));
	if (task == NULL)
		return NULL;

	__kerntask_create(task, thread_func, args);

	task->flags = TASK_FLAGS_KERN;
	task->status = TASK_READY;
	int len = strlen(name);
	memset(task->name, 0x0, 32);
	memcpy(task->name, name, len > 31 ? 31 : len);

	uint64_t cpuid = get_current_cpuid();

	if (current[cpuid] == NULL) {
		current[cpuid] = task;
		task->status = TASK_RUNNING;
		spinlock_unlock(&run_thread_spinlock);
		switch_to(task->rsp);
	}

	sched_enqueue_balanced(task);
	spinlock_unlock(&run_thread_spinlock);
	local_irq_enable();
	return task;
}

static inline int make_elf64_task(char *elf, struct task_struct *task,
                                  const char *pathname)
{
	struct elf64_ehdr *ehdr = (struct elf64_ehdr *)elf;

	/**
	 * 临时映射ELF需要的内存地址进行操作
	 */
	uint64_t last_vaddr;
	uint64_t last_memsz;
	struct elf64_phdr *phdr = (void *)((uint8_t *)elf + ehdr->e_phoff);
	for (int i = 0; i < ehdr->e_phnum; i++) {
		// 这里均假设 LOAD 段都是紧紧相连的
		if (phdr->p_type == PT_LOAD) {
			if (phdr->p_vaddr >= KERNEL_BASE) {
				pr_err("ELF Segment addr > %#llx", KERNEL_BASE);
				return -1;
			}
			// 映射映射 LOAD 段内存并复制 LOAD 段到内存
			uint64_t order = size_to_order(
			        ALIGN_UP((phdr->p_memsz), PAGE_SIZE));
			uint64_t phys = alloc_pages(order);
			size_t len = (ALIGN_UP((phdr->p_memsz), PAGE_SIZE) >> PAGE_SHIFT);
			map_page_range(phys, phdr->p_vaddr, 0x7, len);
			memcpy((void *)phdr->p_vaddr,
			       (const void *)((uint64_t)elf + phdr->p_offset),
			       phdr->p_filesz);
			// 记录最后一个 LOAD 段的地址和大小
			last_vaddr = phdr->p_vaddr;
			last_memsz = phdr->p_memsz;

			struct load_segment_address *new_ls_addr = (struct load_segment_address *)kzalloc(sizeof(struct load_segment_address));
			new_ls_addr->phys = phys;
			new_ls_addr->order = order;
			new_ls_addr->next = NULL;
			if (task->ls_addr == NULL)
			{
				task->ls_addr = new_ls_addr;
			}
			else
			{
				struct load_segment_address *pos = task->ls_addr;

				// 找到鏈表末尾
				while (pos->next)
				{
					pos = pos->next;
				}
				pos->next = new_ls_addr;
			}
		}
		phdr++;
	}

	// 紧接着分配栈空间
	uint64_t stack_down = ALIGN_UP((last_vaddr + last_memsz), PAGE_SIZE);
	uint64_t stack_top = stack_down + USTACK_SIZE;
	uint64_t stack_size = stack_top - stack_down;
	uint64_t stack_order = size_to_order(stack_size);
	uint64_t stack_phys = alloc_pages(stack_order);
	map_page_range(stack_phys, stack_down, 0x7, (stack_size >> PAGE_SHIFT));

	uint64_t start = ((struct elf64_phdr *)((uint8_t *)elf + ehdr->e_phoff))
	                         ->p_vaddr;
	__usertask_create(task, stack_top, (void *)start, (void *)ehdr->e_entry,
	                  0);

	task->stack_basephys = stack_phys;
	task->stack_order = stack_order;
	task->flags = TASK_FLAGS_USER;
	task->status = TASK_READY;
	int len = strlen(pathname);
	memset(task->name, 0x0, 32);
	memcpy(task->name, pathname, len > 31 ? 31 : len);

	// 清除临时映射
	phdr = (void *)((uint8_t *)elf + ehdr->e_phoff);
	for (int i = 0; i < ehdr->e_phnum; i++) {
		if (phdr->p_type == PT_LOAD) {
			size_t len = (ALIGN_UP((phdr->p_memsz), PAGE_SIZE) >> PAGE_SHIFT);
			unmap_page_range(phdr->p_vaddr, len);
		}
		phdr++;
	}
	unmap_page_range(stack_down, (stack_size >> PAGE_SHIFT));
	return 0;
}

static inline int make_elf32_task(int fd, char *elf, struct task_struct *task,
                                  const char *pathname)
{
	fd = fd;
	elf = elf;
	task = task;
	pathname = pathname;
	return 0;
}

int run_process(const char *pathname)
{
	int fd;
	int ret;
	char *elf;
	struct stat sb;
	struct task_struct *task;

	local_irq_disable();
	spinlock_lock(&run_process_spinlock);
	ret = sys_stat(pathname, &sb);
	if (ret < 0) {
		local_irq_enable();
		return -1;
	}

	elf = kzalloc(sb.st_size);
	if (!elf) {
		local_irq_enable();
		return -1;
	}

	fd = sys_open(pathname, FA_READ);
	if (fd >= 0) {
		task = (struct task_struct *)kzalloc(
		        sizeof(struct task_struct));
		sys_read(fd, elf, sb.st_size);
		if (elf[4] == ELFCLASS32) {
			ret = make_elf32_task(fd, elf, task, pathname);
			if (ret < 0) {
				goto err;
			}
		} else if (elf[4] == ELFCLASS64) {
			ret = make_elf64_task(elf, task, pathname);
			if (ret < 0) {
				goto err;
			}
		} else {
			printk("%s: ELF NONE", pathname);
			goto err;
		}
	} else {
		goto err;
	}
	sys_close(fd);
	kfree(elf);
	sched_enqueue_balanced(task);
	spinlock_unlock(&run_process_spinlock);
	local_irq_enable();
	return 0;

err:
	sys_close(fd);
	kfree(elf);
	spinlock_unlock(&run_process_spinlock);
	local_irq_enable();
	return -1;
}

void scheduler(void)
{
	struct task_struct *old_task, *new_task;

	uint64_t cpuid = get_current_cpuid();

	// 这一层判断是为了防止系统启动开启时钟中断后 current
	// 还没被设置导致空指针访问
	if (current[cpuid]) {
		while (!queue_empty(&task_exitqueue[cpuid])) {
			struct task_struct *task = container_of(
			        queue_dequeue(&task_exitqueue[cpuid]),
			        struct task_struct, list);
			switch (task->flags) {
			case TASK_FLAGS_KERN:
				kfree((void *)task->rsp0_base);
				kfree(task->stack);
				kfree(task);
				break;
			case TASK_FLAGS_USER:
				free_pages(task->user_pml4_phys,
				           task->user_pml4_order);
				free_pages(task->user_pdpt_phys,
				           task->user_pdpt_order);
				free_pages(task->user_pd_phys,
				           task->user_pd_order);
				free_pages(task->user_pt_phys,
				           task->user_pt_order);
				free_pages(task->stack_basephys,
				           task->stack_order);
				struct load_segment_address *pos = task->ls_addr;
				while (pos)
				{
					uint64_t phys = pos->phys;
					uint64_t order = pos->order;
					free_pages(phys, order);
					
					struct load_segment_address *old = pos;
					pos = pos->next;
					kfree(old);
				}
				kfree((void *)task->rsp0_base);
				kfree(task);
				break;
			}
		}

		if (!queue_empty(&task_readyqueue[cpuid])) {
			// 就绪队列不为空才进入任务切换
			old_task = current[cpuid];
			new_task = container_of(
			        queue_dequeue(&task_readyqueue[cpuid]),
			        struct task_struct, list);

			new_task->status = TASK_RUNNING;
			switch (old_task->status) {
			// 如果被切换任务是正常运行的TASK_RUNNIN状态才将其移到队列末尾
			case TASK_RUNNING:
				queue_enqueue(&task_readyqueue[cpuid],
				              &old_task->list);
				break;
			case TASK_EXIT:
				// 如果是 EXIT 状态说明这次是主动调用 exit
				// 系列函数 那么就加入 exitqueue
				queue_enqueue(&task_exitqueue[cpuid],
				              &old_task->list);
				break;

			case TASK_BLOCKED:
			case TASK_DEAD:
			case TASK_READY:
			default:
				break;
			}

			// pr_info("Scheduler %u Core task %s", cpuid,
			// new_task->name);
			current[cpuid] = new_task;
			write_cr3(new_task->pml4);
			update_tss_rsp0(new_task->rsp0);
			context_switch(&old_task->rsp, &new_task->rsp);
		}
	}
}

struct task_struct *get_current(void)
{
	uint64_t cpuid = get_current_cpuid();
	return current[cpuid];
}

queue_t *get_task_readyqueue(void)
{
	uint64_t cpuid = get_current_cpuid();
	return &task_readyqueue[cpuid];
}