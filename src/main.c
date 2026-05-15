#include "multiboot2.h"
#include "tty.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "task.h"
#include "pic_8259.h"
#include <stdint.h>
#include <stddef.h>
#include "libc/string.h"
#include "libc/stdlib.h"
#include "panic.h"
#include "mm.h"
#include "printk.h"
#include "syscall.h"
#include "libc/stdio.h"
#include "semaphore.h"
#include "apic.h"

static struct semaphore sem;

void kernel_init000(void *arg)
{
    arg = arg;
    semaphore_p(&sem);
    while (1)
    {
        printk("%s\n", current->name);
    }
}

void kernel_init111(void *arg)
{
    arg = arg;
    while (1)
    {
        semaphore_v(&sem);
        printk("%s\n", current->name);
    }
}

void user_func000(void *arg)
{
    arg = arg;
    uint32_t retval;
    uint32_t arg_list[6];
    arg_list[0] = SYSCALL_WRITE;
    arg_list[1] = STDOUT;
    arg_list[2] = (uint32_t)arg;
    arg_list[3] = 0x5;
    arg_list[4] = 0x0;
    arg_list[5] = 0x0;
    while (1)
    {
        syscall(arg_list, retval);
    }
}

void user_func111(void *arg)
{
    arg = arg;
    uint32_t retval;
    uint32_t arg_list[6];
    arg_list[0] = SYSCALL_WRITE;
    arg_list[1] = STDOUT;
    arg_list[2] = (uint32_t)arg;
    arg_list[3] = 0x5;
    arg_list[4] = 0x0;
    arg_list[5] = 0x0;
    while (1)
    {
        syscall(arg_list, retval);
    }
}

void kernel_init(void *arg)
{
    arg = arg;

    semaphore_init(&sem, 0);
    ktask_create(kernel_init000, 0, "KERNEL_TASK 0");
    ktask_create(kernel_init111, 0, "KERNEL_TASK 1");
    utask_create(user_func000, "USER_TASK 0\n", "user_func000");
    utask_create(user_func111, "USER_TASK 1\n", "user_func111");
    while (1)
    {
        printk("%s, total_tasks %u\n", current->name, total_tasks());
    }
}

void start_kernel(void)
{
    // step 1.
    early_page_init();
    total_memory_init();
    early_mm_init();

    // step 2.
    gdt_init();
    idt_init();
    tty_init();
    page_init();
    mm_init();
    task_init();
    apic_init();

    // Never return
    ktask_create(kernel_init, 0, "kernel_init");
    PANIC("PANIC");
}
