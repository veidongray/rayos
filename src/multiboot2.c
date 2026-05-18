#include "multiboot2.h"

static uint64_t total_mem = 0;

extern uint32_t _mboot_info[];
extern uint32_t _mboot_magic[];

void parse_multiboot2_mmap(void *mbi_addr)
{
    uint8_t *ptr = (uint8_t *)mbi_addr + 8; // skip total_size + reserved
    uint8_t *end = (uint8_t *)mbi_addr + *(uint32_t *)mbi_addr;

    while (ptr < end)
    {
        struct multiboot2_tag *tag = (struct multiboot2_tag *)ptr;
        if (tag->type == 0)
            break; // end tag

        if (tag->type == MULTIBOOT2_TAG_MMAP)
        {
            uint8_t *entry_ptr = ptr + 16; // skip type, size, entry_size, version
            uint8_t *entry_end = ptr + tag->size;

            while (entry_ptr < entry_end)
            {
                struct multiboot2_mmap_entry *e = (struct multiboot2_mmap_entry *)entry_ptr;
                if (e->type == 1)
                { // available memory
                    total_mem += e->len;
                }
                entry_ptr += 24; // sizeof(multiboot2_mmap_entry)
            }

            // total_mem is your physical RAM size in bytes!
            // You can store it globally or print it via serial
        }

        ptr += (tag->size + 7) & ~7; // align to 8-byte boundary
    }
}

uint64_t get_total_mem(void)
{
    return total_mem;
}

void total_memory_init(void)
{
    if (_mboot_magic[0] == 0x36d76289)
    {
        parse_multiboot2_mmap((void *)((uint64_t)_mboot_info[0]));
    }
}