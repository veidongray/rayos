#include "gdt.h"
#include "print.h"

static struct gdt_entry descriptors[5] __attribute__((aligned(4096)));
static struct gdtr gdtr __attribute__((aligned(4096)));

int create_gdt_entry(struct gdt_entry *entry, uint32_t base,
    uint32_t limit, uint8_t access, uint8_t granularity) {
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
    struct gdtr *g = &gdtr;
    g->limit = size - 1;
    g->base = (uint32_t)gdt;

    asm volatile ("lgdt %0" : : "m"(g));
    return 0;
}

int gdt_init(void)
{
    struct gdt_entry *desc = descriptors;
    create_gdt_entry(&desc[0], 0, 0, 0, 0);
    create_gdt_entry(&desc[1], 0, 0xFFFFF, 0x9A, 0xCF);  // Kernel mode code segment
    create_gdt_entry(&desc[2], 0, 0xFFFFF, 0x93, 0xCF);  // Kernel mode data segment
    load_gdt(desc, sizeof(struct gdt_entry) * 3);
	extern void gdt_flush(struct gdtr *gdtr);
    gdt_flush(&gdtr);
    return 0;
}