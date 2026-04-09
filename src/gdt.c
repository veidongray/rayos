#include "gdt.h"
#include "print.h"

static struct gdt_entry descriptors[5];
struct gdt_entry *get_gdt_entry(void) { return descriptors; }
static struct gdtr gdtr;
struct gdtr *get_gdtr(void) { return &gdtr; }

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
    struct gdtr *g = get_gdtr();
    g->limit = size - 1;
    g->base = (uint32_t)gdt;

    asm volatile ("lgdt %0" : : "m"(g));
    return 0;
}

int gdt_init(void)
{
    struct gdt_entry *desc = get_gdt_entry();
    create_gdt_entry(&desc[0], 0, 0, 0, 0);
    create_gdt_entry(&desc[1], 0, 0xFFFFF, 0x9A, 0xCF);  // Kernel mode code segment
    create_gdt_entry(&desc[2], 0, 0xFFFFF, 0x93, 0xCF);  // Kernel mode data segment
    load_gdt(desc, sizeof(struct gdt_entry) * 3);
	extern void gdt_flush(struct gdtr *gdtr);
    gdt_flush(get_gdtr());
    cga_info("GDT loaded with 5 entries.\n");
    // for (int i = 0; i < 5; ++i) {
    //     cga_printf("GDT Entry %d: Base=0x%X, Limit=0x%X, Access=0x%X, Granularity=0x%X\n",
    //                i, (descriptors[i].base_high << 24) | (descriptors[i].base_mid << 16) | descriptors[i].base_low,
    //                descriptors[i].limit, descriptors[i].access, descriptors[i].granularity);
    // }
    return 0;
}