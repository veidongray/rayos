#ifndef GDT_H
#define GDT_H

#include <stdint.h>

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

#endif // GDT_H