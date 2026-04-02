#include <stdint.h>

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


uint8_t *vga = (uint8_t *)0xB8000;

int putc(char ch)
{
    if (ch >= 32) {
        *vga = ch;
        vga += 2;
        if (vga >= (uint8_t *)(0xB8000 + 4000)) {
            vga = (uint8_t *)0xB8000;
        }
    }
    return ch;
}

int puts(char *str)
{
    int i;
    for (i = 0; i < 2000 && str[i] != '\0'; ++i) {
        putc(str[i]);
    }
    return i;
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
    while (1) {
        asm volatile ("hlt\r\n");
    }
}
