#ifndef GDT_H
#define GDT_H

#include <stdint.h>
#define KCODE_SELECTOR (0x08UL | 0x0UL)
#define KDATA_SELECTOR (0x10UL | 0x0UL)
#define UCODE_SELECTOR (0x18UL | 0x3UL)
#define UDATA_SELECTOR (0x20UL | 0x3UL)
#define TSS_SELECTOR (0x28UL | 0x0UL)

struct gdt_entry {
    uint16_t limit;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

int create_gdt_entry(struct gdt_entry *entry, uint32_t base,
    uint32_t limit, uint8_t access, uint8_t granularity);
int load_gdt(struct gdt_entry *gdt, uint16_t size);
struct gdtr *get_gdtr(void);
int gdt_init(void);
void update_tss_esp0(uint32_t esp0);
uint32_t get_current_esp(void);

#endif // GDT_H