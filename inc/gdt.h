#ifndef GDT_H
#define GDT_H

#include <stdint.h>
#define KCODE_SELECTOR (0x08UL | 0x0UL)
#define KDATA_SELECTOR (0x10UL | 0x0UL)
#define UCODE_SELECTOR (0x18UL | 0x3UL)
#define UDATA_SELECTOR (0x20UL | 0x3UL)
#define TSS_SELECTOR (0x28UL | 0x0UL)

struct gdtr64
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct tss_entry
{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

void gdt_init(void);
void update_tss_rsp0(uint64_t rsp0);

#endif // GDT_H