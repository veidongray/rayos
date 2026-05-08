#include "multiboot2.h"
#include "print.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "kheap.h"
#include "task.h"
#include "pic_8259.h"
#include <stdint.h>
#include <stddef.h>
#include "libc/string.h"
#include "libc/stdlib.h"
#include "panic.h"

extern uint32_t _mboot_info[];
extern uint32_t _mboot_magic[];
extern uint32_t host_total_mem;

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
    if (_mboot_magic[0] == 0x36d76289)
        parse_multiboot2_mmap((uint32_t *)_mboot_info[0]);
    else
    {
        // If we don't have a valid multiboot magic number, we can't trust the bootloader and should halt
        PANIC("Lost Bootloader...\n");
    }

    // Check if we have at least 8MB of memory, otherwise we can't do much
    if (host_total_mem < 0x800000)
    {
        PANIC("Not enough memory detected: %u bytes\n", host_total_mem);
    }
    gdt_init();
    idt_init();
    cga_init();
    page_init();
    kheap_init();
    task_init();

    // Never return
    ktask_create(kernel_init, 0, "kernel_init");
    PANIC("PANIC");
}
