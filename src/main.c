#include <stdint.h>
#include "print.h"
#include "pic.h"
#include "gdt.h"
#include "idt.h"
#include "page.h"
#include "multiboot2.h"

extern uint32_t _mboot_info[];
extern uint32_t _mboot_magic[];
extern uint32_t _boot_page_directory[];
extern uint32_t _kernel_end_aligned[];

struct page {
    physaddr_t base;
    uint32_t flags;
    uint32_t kref;
};

struct free_page {
    struct page page;
    struct free_page *next;
};
uint32_t free_page_count = 0;
struct free_page *free_page_list = (struct free_page *)0;

struct task_struct {
    uint32_t esp;
    uint32_t eip;
    uint32_t cr3;
    struct task_struct *next;
};


uint32_t task0_stack[1024] = {0};
struct task_struct task0 = {
    .esp = (uint32_t)task0_stack + sizeof(task0_stack) - (4 * 9),
    .eip = 0,
    .cr3 = 0,
    .next = 0,
};

uint32_t task1_stack[1024] = {0};
struct task_struct task1 = {
    .esp = (uint32_t)task1_stack + sizeof(task1_stack) - (4 * 9),
    .eip = 0,
    .cr3 = 0,
    .next = 0,
};

struct task_struct *current_task = 0;
uint32_t ticks = 0;
void task0_func(void)
{
    while (1)
    {
        // cga_info("Task0000 is running. ticks = %u\n", ticks++);
        context_switch(current_task, current_task->next);
        asm volatile("hlt\r\n");
    }
}

void task1_func(void)
{
    while (1)
    {
        // cga_info("Task1111 is running. ticks = %u\n", ticks++);
        context_switch(current_task->next, current_task);
        asm volatile("hlt\r\n");
    }
}

void kernel_main(void)
{
    uint32_t i = 0;
    gdt_init();
    task0_stack[1015] = (uint32_t)task0_func;
    task1_stack[1015] = (uint32_t)task1_func;
    task0.eip = (uint32_t)task0_func;
    task1.eip = (uint32_t)task1_func;
    task0.cr3 = (uint32_t)_boot_page_directory - 0xc0000000; // identity-mapped page directory
    task1.cr3 = (uint32_t)_boot_page_directory - 0xc0000000; // identity-mapped page directory
    task0.next = &task1;
    task1.next = &task0;
    current_task = &task0;
    idt_init();
    cga_init();
    extern void switch_to_task(struct task_struct *task);
    extern void context_switch(struct task_struct *current_task, struct task_struct *next_task);
    cga_info("current_task = 0x%X.\n", (uint32_t)current_task);
    cga_info("current_task->next = 0x%X.\n", (uint32_t)current_task->next);
    switch_to_task(current_task);
    
    while (1)
    {
        asm volatile("hlt\r\n");
    }
}

physaddr_t alloc_page(void)
{
    uint32_t i;
    extern struct free_page *free_page_list;
    physaddr_t page = (physaddr_t)-1;

    page = free_page_list[0].page.base;
    free_page_list = &free_page_list[1];
    return page;
}

void main(void)
{
    _boot_page_directory[0] = 0x2;
    if (_mboot_magic[0] != 0x36d76289)
    {
        cga_info("Invalid multiboot magic number: 0x%X.\n", _mboot_magic[0]);
        while (1)
        {
            asm volatile("hlt\r\n");
        }
    }
    parse_multiboot2_mmap((void*)_mboot_info[0]);
    kernel_main();
}
