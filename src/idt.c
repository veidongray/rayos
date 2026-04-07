#include "idt.h"
#include "pic.h"
#include "print.h"

static struct idt_entry idt[256];
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
    extern void *isr_stub_table[];
    for (int i = 0; i <= 47; ++i) {
        set_idt_entry(i, isr_stub_table[i], 0x8E);
    }
    load_idt(idt, sizeof(idt));
    cga_info("IDT loaded with 32 entries.\n");
    // for (int i = 0; i <= 47; ++i) {
    //     cga_printf("IDT Entry %d: ISR Address=0x%X\n", i, (idt[i].isr_high << 16) | idt[i].isr_low);
    // }
    pic_remap(0x20, 0x28);
    timer_init();
    enable_irq();
    return 0;
}