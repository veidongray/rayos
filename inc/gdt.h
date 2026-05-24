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

void gdt_init(void);

#endif // GDT_H