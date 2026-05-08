#include "multiboot2.h"
#include "print.h"
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

void user_init000(void *arg)
{
    arg = arg;
    while (1)
    {
        cga_printf("%s\n", current->name);
        panic_halt();
        return;
    }
}

void user_init111(void *arg)
{
    arg = arg;
    while (1)
    {
        cga_printf("%s\n", current->name);
        panic_halt();
        return;
    }
}

void user_init222(void *arg)
{
    arg = arg;
    while (1)
    {
        cga_printf("%s\n", current->name);
        panic_halt();
        return;
    }
}

void user_init333(void *arg)
{
    arg = arg;
    while (1)
    {
        cga_printf("%s\n", current->name);
        panic_halt();
        return;
    }
}

void kernel_init(void *arg)
{
    arg = arg;

    ktask_create(user_init000, 0, "user_init000");
    ktask_create(user_init111, 0, "user_init111");
    ktask_create(user_init222, 0, "user_init222");
    ktask_create(user_init333, 0, "user_init333");
    while (1)
    {
        cga_printf("%s\n", current->name);
        panic_halt();
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
