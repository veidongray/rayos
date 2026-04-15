#include "irq.h"
#include "print.h"

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
    load_idt(idt, sizeof(idt));
    return 0;
}