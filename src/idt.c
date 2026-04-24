#include "idt.h"
#include "pic_8259.h"
#include "print.h"
#include "task.h"

static struct idt_entry idt[256] __attribute__((aligned(4096)));
void set_idt_entry(uint8_t vector, void* isr, uint8_t flags) {
    struct idt_entry* descriptor = &idt[vector];

    descriptor->isr_low        = (uint32_t)isr & 0xFFFF;
    descriptor->kernel_cs      = 0x08; // this value can be whatever offset your kernel code selector is in your GDT
    descriptor->attributes     = flags;
    descriptor->isr_high       = (uint32_t)isr >> 16;
    descriptor->reserved       = 0;
}

void load_idt(struct idt_entry* idt, uint16_t size) {
    struct idtr idtr;
    idtr.limit = size - 1;
    idtr.base = (uint32_t)idt;

    asm volatile ("lidt %0" : : "m"(idtr));
}

int idt_init(void)
{
    uint32_t i;

    extern void default_isr(void);
    for (i = 0; i < 256; ++i)
        set_idt_entry(i, default_isr, 0);

    // Set up the PIC timer interrupt (IRQ0).
    extern void isr_pic_timer(void);
    set_idt_entry(IRQ0_VECTOR, isr_pic_timer, 0x8E); // Present, ring 0, 32-bit interrupt gate
    // Set up the page fault handler (interrupt vector 14).
    extern void isr_page_fault(void);
    set_idt_entry(14, isr_page_fault, 0x8E);

    load_idt(idt, sizeof(idt));
    pic_remap(0x20, 0x28);
    timer_init();
    enable_irq();
    return 0;
}

void enable_irq(void)
{
    asm volatile ("sti");
}

void disable_irq(void)
{
    asm volatile ("cli");
}

void timer_interrupt_handler(void)
{
    pic_sendEOI(0); // Send End of Interrupt (EOI) signal to PIC
    scheduler();
}

void page_fault_handler(uint32_t error_code)
{
    uint32_t faulting_address;
    asm volatile ("mov %%cr2, %0" : "=r" (faulting_address)); // Get the faulting address from CR2

    cga_printf("Page Fault! Error code: %x, Faulting address: %x\n", error_code, faulting_address);
    while (1) asm volatile("cli\r\nhlt\r\n");
}