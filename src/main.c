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

void user_init000(void *arg)
{
    arg = arg;
    while (1)
    {
        printk("%s\n", current->name);
    }
}

void user_init111(void *arg)
{
    arg = arg;
    while (1)
    {
        printk("%s\n", current->name);
    }
}

void user_func(void *arg)
{
    arg = arg;
    asm volatile(
        "pushl %eax\r\n"
        "movl $0x100, %eax\r\n"
        // "int $0x80\r\n"
        "popl %eax\r\n"
    );
    while (1);
}

void kernel_init(void *arg)
{
    uint32_t i;
    arg = arg;

    ktask_create(user_init000, 0, "user_init000");
    ktask_create(user_init111, 0, "user_init111");
    utask_create(user_func, 0, "user_func");
    utask_create(user_func, 0, "user_func");
    while (1)
    {
        printk("%s, total_tasks %u\n", current->name, total_tasks());
    }
}

void start_kernel(void)
{
    // step 1.
    early_page_init();
    early_mm_init();

    // step 2.
    gdt_init();
    idt_init();
    tty_init();
    page_init();
    mm_init();
    task_init();

    // Never return
    ktask_create(kernel_init, 0, "kernel_init");
    PANIC("PANIC");
}
