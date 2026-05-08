#include "gdt.h"
#include "print.h"
#include "libc/string.h"

static struct tss_entry tss;
static struct gdt_entry descriptors[6] __attribute__((aligned(4096)));
static struct gdtr gdtr __attribute__((aligned(4096)));

int create_gdt_entry(struct gdt_entry *entry, uint32_t base,
                     uint32_t limit, uint8_t access, uint8_t granularity)
{
    entry->base_low = base & 0xFFFF;
    entry->base_mid = (base >> 16) & 0xFF;
    entry->base_high = (base >> 24) & 0xFF;
    entry->limit = limit & 0xFFFF;
    entry->granularity = (limit >> 16) & 0x0F;
    entry->granularity |= granularity & 0xF0;
    entry->access = access;
    return 0;
}

int load_gdt(struct gdt_entry *gdt, uint16_t size)
{
    struct gdtr *g = &gdtr;
    g->limit = size - 1;
    g->base = (uint32_t)gdt;

    asm volatile("lgdt %0" : : "m"(g));
    return 0;
}

int gdt_init(void)
{
    struct gdt_entry *desc = descriptors;
    create_gdt_entry(&desc[0], 0, 0, 0, 0);
    create_gdt_entry(&desc[1], 0, 0xFFFFF, 0x9A, 0xCF); // Kernel code
    create_gdt_entry(&desc[2], 0, 0xFFFFF, 0x92, 0xCF); // Kernel data
    create_gdt_entry(&desc[3], 0, 0xFFFFF, 0xFA, 0xCF); // User code
    create_gdt_entry(&desc[4], 0, 0xFFFFF, 0xF2, 0xCF); // User data
    create_gdt_entry(&desc[5], (uint32_t)&tss, sizeof(tss) - 1, 0x89, 0x00);

    load_gdt(desc, sizeof(struct gdt_entry) * 6);
    gdt_flush(&gdtr);

    tss.prev_tss = 0;
    tss.esp0 = 0x0;
    tss.ss0 = KDATA_SELECTOR;
    tss.esp1 = 0;
    tss.ss1 = 0;
    tss.esp2 = 0;
    tss.ss2 = 0;
    tss.cr3 = 0;
    tss.eip = 0;
    tss.eflags = 0;
    tss.eax = 0;
    tss.ecx = 0;
    tss.edx = 0;
    tss.ebx = 0;
    tss.esp = 0;
    tss.ebp = 0;
    tss.esi = 0;
    tss.edi = 0;
    tss.es = 0;
    tss.cs = 0;
    tss.ss = 0;
    tss.ds = 0;
    tss.fs = 0;
    tss.gs = 0;
    tss.ldt = 0;
    tss.trap = 0;
    // 关键：设置 iomap_base > limit，表示无 I/O 权限位图
    tss.iomap_base = sizeof(tss); // e.g., 104 if TSS is 104 bytes

    asm volatile("ltr %%ax" ::"a"(TSS_SELECTOR));
    return 0;
}

uint32_t get_current_esp(void)
{
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    return esp;
}

void update_tss_esp0(uint32_t esp0)
{
    tss.esp0 = esp0;
}