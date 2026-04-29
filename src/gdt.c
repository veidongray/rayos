#include "gdt.h"
#include "kheap.h"
#include "print.h"
#include "libc/string.h"

struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

static struct tss_entry tss;

static struct gdt_entry descriptors[6] __attribute__((aligned(4096)));
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
    create_gdt_entry(&desc[3], 0, 0xFFFFF, 0xFA, 0xCF);  // User mode code segment
    create_gdt_entry(&desc[4], 0, 0xFFFFF, 0xF3, 0xCF);  // User mode data segment
    create_gdt_entry(&desc[5], (uint32_t)&tss, sizeof(tss) - 1, 0x89, 0x00);
    load_gdt(desc, sizeof(struct gdt_entry) * 6);
	extern void gdt_flush(struct gdtr *gdtr);
    gdt_flush(&gdtr);
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = KDATA_SELECTOR;
    tss.esp0 = (uint32_t)kmalloc(8192, KHEAP_ALLOC);
    asm volatile ("ltr %%ax" :: "a"(TSS_SELECTOR));
    return 0;
}

uint32_t get_current_esp(void)
{
    uint32_t esp;
    asm volatile ("mov %%esp, %0" : "=r"(esp));
    return esp;
}

void update_tss_esp0(uint32_t esp0)
{
    tss.esp0 = esp0;
    // 重新加载 TR 寄存器以使更改生效
    asm volatile ("ltr %%ax" :: "a"(TSS_SELECTOR));
}