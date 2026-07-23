#ifndef GDT_H
#define GDT_H

#include <types.h>

#define KCODE_SELECTOR (0x08UL | 0x0UL)
#define KDATA_SELECTOR (0x10UL | 0x0UL)
#define UCODE_SELECTOR (0x18UL | 0x3UL)
#define UDATA_SELECTOR (0x20UL | 0x3UL)
#define TSS_SELECTOR (0x28UL | 0x0UL)

struct gdtr64 {
	__u16 limit;
	__u64 base;
} __attribute__((packed));

struct tss_entry {
	__u32 reserved0;
	__u64 rsp0;
	__u64 rsp1;
	__u64 rsp2;
	__u64 reserved1;
	__u64 ist1;
	__u64 ist2;
	__u64 ist3;
	__u64 ist4;
	__u64 ist5;
	__u64 ist6;
	__u64 ist7;
	__u64 reserved2;
	__u16 reserved3;
	__u16 iomap_base;
} __attribute__((packed));

void gdt_init(void);
void ap_gdt_init(void);
void update_tss_rsp0(__u64 rsp0);

#endif // GDT_H