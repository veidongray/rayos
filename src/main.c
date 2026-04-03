#include <stdint.h>
#include "print.h"

struct gdt_entry {
    uint16_t limit;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gtdr{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_descriptor;

struct idt_entry {
	uint16_t    isr_low;      // The lower 16 bits of the ISR's address
	uint16_t    kernel_cs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t     reserved;     // Set to zero
	uint8_t     attributes;   // Type and attributes; see the IDT page
	uint16_t    isr_high;     // The higher 16 bits of the ISR's address
} __attribute__((packed));

struct idtr {
	uint16_t	limit;
	uint32_t	base;
} __attribute__((packed));

int create_gdt_entry(struct gdt_entry *entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
    entry->base_low = base & 0xFFFF;
    entry->base_mid = (base >> 16) & 0xFF;
    entry->base_high = (base >> 24) & 0xFF;
    entry->limit = limit & 0xFFFF;
    entry->granularity = (limit >> 16) & 0x0F;
    entry->granularity |= granularity & 0xF0;
    entry->access = access;
    return 0;
}

int load_gdt(struct gdt_entry *gdt, uint16_t size) {
    gdt_descriptor.limit = size - 1;
    gdt_descriptor.base = (uint32_t)gdt;

    asm volatile ("lgdt %0" : : "m"(gdt_descriptor));
    return 0;
}

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

void main(void)
{
    struct gdt_entry descriptors[5];
    create_gdt_entry(&descriptors[0], 0, 0, 0, 0);
    create_gdt_entry(&descriptors[1], 0, 0xFFFFF, 0x9A, 0xCF);  // Kernel mode code segment
    create_gdt_entry(&descriptors[2], 0, 0xFFFFF, 0x92, 0xCF);  // Kernel mode data segment
    create_gdt_entry(&descriptors[3], 0, 0xFFFFF, 0xFA, 0xCF);  // User mode code segment
    create_gdt_entry(&descriptors[4], 0, 0xFFFFF, 0xF2, 0xCF);  // User mode code segment
    load_gdt(descriptors, sizeof(descriptors));
	extern void gdt_flush(struct gtdr *gdtr);
    gdt_flush(&gdt_descriptor);
    cga_printf("GDT initialization...\n");
    for (int i = 0; i < 5; ++i) {
        cga_printf("GDT Entry %d: Base=0x%X, Limit=0x%X, Access=0x%X, Granularity=0x%X\n",
                   i, (descriptors[i].base_high << 24) | (descriptors[i].base_mid << 16) | descriptors[i].base_low,
                   descriptors[i].limit, descriptors[i].access, descriptors[i].granularity);
    }

    extern void *isr_stub_table[];
    for (int i = 0; i < 32; ++i) {
        set_idt_entry(i, isr_stub_table[i], 0x8E);
    }
    load_idt(idt, sizeof(idt));
    cga_printf("IDT loaded with 32 entries.\n");
    for (int i = 0; i < 32; ++i) {
        cga_printf("IDT Entry %d: ISR Address=0x%X\n", i, (idt[i].isr_high << 16) | idt[i].isr_low);
    }
    asm volatile ("sti"); // Enable interrupts
    
    while (1) {
        asm volatile ("hlt\r\n");
    }
}
