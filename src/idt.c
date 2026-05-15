#include "idt.h"
#include "pic_8259.h"
#include "tty.h"
#include "task.h"
#include "paging.h"
#include "gdt.h"
#include "panic.h"
#include <stddef.h>
#include "apic.h"

static struct idt_entry idt[256] __attribute__((aligned(4096)));
void set_idt_entry(uint8_t vector, void *isr, uint8_t flags)
{
    struct idt_entry *descriptor = &idt[vector];

    descriptor->isr_low = (uint32_t)isr & 0xFFFF;
    descriptor->kernel_cs = KCODE_SELECTOR;
    descriptor->attributes = flags;
    descriptor->isr_high = (uint32_t)isr >> 16;
    descriptor->reserved = 0;
}

void load_idt(struct idt_entry *idt, uint16_t size)
{
    struct idtr idtr;
    idtr.limit = size - 1;
    idtr.base = (uint32_t)idt;

    asm volatile("lidt %0" : : "m"(idtr));
}

extern void default_isr(void);
extern void isr_pic_timer(void);
extern void isr_page_fault(void);
extern void isr_double_fault(void);
extern void isr_gp_fault(void);
extern void isr_syscall(uint32_t);

int idt_init(void)
{
    uint32_t i;

    for (i = 0; i < 256; ++i)
        set_idt_entry(i, default_isr, 0);

    // Set up the PIC timer interrupt (IRQ0).
    set_idt_entry(IRQ0_VECTOR, isr_pic_timer, 0x8E); // Present, ring 0, 32-bit interrupt gate

    // Set up the page fault handler (interrupt vector 14).
    set_idt_entry(14, isr_page_fault, 0x8E);

    // Set up the double fault handler (interrupt vector 8).
    set_idt_entry(8, isr_double_fault, 0x8E);

    // Set up the GP fault handler (interrupt vector 13).
    set_idt_entry(13, isr_gp_fault, 0x8E);

    // Set up the system call handler (interrupt vector 128).
    // DPL=3
    set_idt_entry(128, isr_syscall, 0xEE);

    load_idt(idt, sizeof(idt));
    pic_remap(0x20, 0x28);
    timer_init();
    return 0;
}

int is_interrupts_enabled(void)
{
    unsigned long flags;
    asm volatile("pushf; pop %0" : "=rm"(flags));
    return !!(flags & (1UL << 9));
}

void timer_interrupt_handler(void)
{
    pic_sendEOI(0); // Send End of Interrupt (EOI) signal to PIC
    lapic_write(LAPIC_EOI, 0);
    scheduler();
}

void page_fault_handler(uint32_t error_code)
{
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address)); // Get the faulting address from CR2
    PANIC("Page Fault! Error code: 0x%x, Faulting address: 0x%x\n", error_code, faulting_address);
}

void double_fault_handler(void)
{
    PANIC("DOUBLE FAULT! System halted.\n");
}

void gp_fault_handler(uint32_t error_code)
{
    PANIC("GENERAL PROTECTION FAULT! Error code: 0x%x\n", error_code);
}