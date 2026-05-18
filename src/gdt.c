#include <stdint.h>
#include "gdt.h"
#include "aligned.h"

// Each define here is for a specific flag in the descriptor.
// Refer to the intel documentation for a description of what each one does.
#define SEG_DESCTYPE(x) ((x) << 0x04)      // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x) ((x) << 0x07)          // Present
#define SEG_SAVL(x) ((x) << 0x0C)          // Available for system use
#define SEG_LONG(x) ((x) << 0x0D)          // Long mode
#define SEG_SIZE(x) ((x) << 0x0E)          // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x) ((x) << 0x0F)          // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
#define SEG_PRIV(x) (((x) & 0x03) << 0x05) // Set privilege level (0 - 3)

#define SEG_DATA_RD 0x00        // Read-Only
#define SEG_DATA_RDA 0x01       // Read-Only, accessed
#define SEG_DATA_RDWR 0x02      // Read/Write
#define SEG_DATA_RDWRA 0x03     // Read/Write, accessed
#define SEG_DATA_RDEXPD 0x04    // Read-Only, expand-down
#define SEG_DATA_RDEXPDA 0x05   // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD 0x06  // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_EX 0x08        // Execute-Only
#define SEG_CODE_EXA 0x09       // Execute-Only, accessed
#define SEG_CODE_EXRD 0x0A      // Execute/Read
#define SEG_CODE_EXRDA 0x0B     // Execute/Read, accessed
#define SEG_CODE_EXC 0x0C       // Execute-Only, conforming
#define SEG_CODE_EXCA 0x0D      // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC 0x0E     // Execute/Read, conforming
#define SEG_CODE_EXRDCA 0x0F    // Execute/Read, conforming, accessed

#define GDT_CODE_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                         SEG_LONG(0) | SEG_SIZE(1) | SEG_GRAN(1) | \
                         SEG_PRIV(0) | SEG_CODE_EXRD

#define GDT_DATA_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                         SEG_LONG(0) | SEG_SIZE(1) | SEG_GRAN(1) | \
                         SEG_PRIV(0) | SEG_DATA_RDWR

#define GDT_CODE_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                         SEG_LONG(0) | SEG_SIZE(1) | SEG_GRAN(1) | \
                         SEG_PRIV(3) | SEG_CODE_EXRD

#define GDT_DATA_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                         SEG_LONG(0) | SEG_SIZE(1) | SEG_GRAN(1) | \
                         SEG_PRIV(3) | SEG_DATA_RDWR

#define GDT_CODE64_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                           SEG_LONG(1) | SEG_SIZE(0) | SEG_GRAN(1) | \
                           SEG_PRIV(0) | SEG_CODE_EXRD

#define GDT_DATA64_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                           SEG_LONG(0) | SEG_SIZE(1) | SEG_GRAN(1) | \
                           SEG_PRIV(0) | SEG_DATA_RDWR

#define GDT_CODE64_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                           SEG_LONG(1) | SEG_SIZE(0) | SEG_GRAN(1) | \
                           SEG_PRIV(3) | SEG_CODE_EXRD

#define GDT_DATA64_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                           SEG_LONG(0) | SEG_SIZE(1) | SEG_GRAN(1) | \
                           SEG_PRIV(3) | SEG_DATA_RDWR

uint64_t create_descriptor(uint32_t base, uint32_t limit, uint16_t flag)
{
    uint64_t descriptor;

    // Create the high 32 bit segment
    descriptor = limit & 0x000F0000;         // set limit bits 19:16
    descriptor |= (flag << 8) & 0x00F0FF00;  // set type, p, dpl, s, g, d/b, l and avl fields
    descriptor |= (base >> 16) & 0x000000FF; // set base bits 23:16
    descriptor |= base & 0xFF000000;         // set base bits 31:24

    // Shift by 32 to allow for low part of segment
    descriptor <<= 32;

    // Create the low 32 bit segment
    descriptor |= base << 16;         // set base bits 15:0
    descriptor |= limit & 0x0000FFFF; // set limit bits 15:0

    return descriptor;
}

ALIGN_ATTR(4096)
static uint64_t gdt[5];
ALIGN_ATTR(4096)
static struct gdtr64 g;
ALIGN_ATTR(4096)
static struct tss_entry tss;

void gdt_init(void)
{
    gdt[0] = create_descriptor(0, 0, 0);
    gdt[1] = create_descriptor(0, 0xfffff, GDT_CODE64_PL0);
    gdt[2] = create_descriptor(0, 0xfffff, GDT_DATA64_PL0);
    gdt[3] = create_descriptor(0, 0xfffff, GDT_CODE64_PL3);
    gdt[4] = create_descriptor(0, 0xfffff, GDT_DATA64_PL3);

    g.limit = sizeof(gdt) - 1;
    g.base = (uint64_t)gdt;

    asm volatile("lgdt %0" : : "m"(g));
    asm volatile(
        "movq %0, %%rax\r\n"
        "movw %%ax, %%ds\r\n"
        "movw %%ax, %%es\r\n"
        "movw %%ax, %%fs\r\n"
        "movw %%ax, %%gs\r\n"
        "movw %%ax, %%ss\r\n"
        :
        : "r"(KDATA_SELECTOR)
        : "rax");
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