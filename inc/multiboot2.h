#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include <stdint.h>

#define MULTIBOOT2_TAG_MMAP 6

struct multiboot2_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot2_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed));

void parse_multiboot2_mmap(void* mbi_addr);
void total_memory_init(void);
uint64_t get_total_mem(void);
#endif // MULTIBOOT2_H