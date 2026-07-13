#include <gdt.h>
#include <printf.h>
#include <types.h>

// Each define here is for a specific flag in the descriptor.
// Refer to the intel documentation for a description of what each one does.
#define SEG_DESCTYPE(x)                                                        \
	((x) << 0x04) // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x) ((x) << 0x07) // Present
#define SEG_SAVL(x) ((x) << 0x0C) // Available for system use
#define SEG_LONG(x) ((x) << 0x0D) // Long mode
#define SEG_SIZE(x) ((x) << 0x0E) // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x)                                                            \
	((x) << 0x0F) // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
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

#define GDT_CODE_PL0                                                           \
	SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) |            \
	        SEG_SIZE(1) | SEG_GRAN(1) | SEG_PRIV(0) | SEG_CODE_EXRD

#define GDT_DATA_PL0                                                           \
	SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) |            \
	        SEG_SIZE(1) | SEG_GRAN(1) | SEG_PRIV(0) | SEG_DATA_RDWR

#define GDT_CODE_PL3                                                           \
	SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) |            \
	        SEG_SIZE(1) | SEG_GRAN(1) | SEG_PRIV(3) | SEG_CODE_EXRD

#define GDT_DATA_PL3                                                           \
	SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) |            \
	        SEG_SIZE(1) | SEG_GRAN(1) | SEG_PRIV(3) | SEG_DATA_RDWR

#define GDT_CODE64_PL0                                                         \
	SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(1) |            \
	        SEG_SIZE(0) | SEG_GRAN(1) | SEG_PRIV(0) | SEG_CODE_EXRD

#define GDT_DATA64_PL0                                                         \
	SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) |            \
	        SEG_SIZE(1) | SEG_GRAN(1) | SEG_PRIV(0) | SEG_DATA_RDWR

#define GDT_CODE64_PL3                                                         \
	SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(1) |            \
	        SEG_SIZE(0) | SEG_GRAN(1) | SEG_PRIV(3) | SEG_CODE_EXRD

#define GDT_DATA64_PL3                                                         \
	SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) |            \
	        SEG_SIZE(1) | SEG_GRAN(1) | SEG_PRIV(3) | SEG_DATA_RDWR

__attribute__((aligned(4096))) static __u64 gdt[7];
__attribute__((aligned(4096))) static struct gdtr64 g;
__attribute__((aligned(4096))) static struct tss_entry tss;
static __u8 kernel_stack[16384] __attribute__((aligned(16)));

__u64 create_descriptor(__u32 base, __u32 limit, uint16_t flag)
{
	__u64 descriptor;

	// Create the high 32 bit segment
	descriptor = limit & 0x000F0000; // set limit bits 19:16
	descriptor |=
	        (flag << 8) &
	        0x00F0FF00; // set type, p, dpl, s, g, d/b, l and avl fields
	descriptor |= (base >> 16) & 0x000000FF; // set base bits 23:16
	descriptor |= base & 0xFF000000;         // set base bits 31:24

	// Shift by 32 to allow for low part of segment
	descriptor <<= 32;

	// Create the low 32 bit segment
	descriptor |= base << 16;         // set base bits 15:0
	descriptor |= limit & 0x0000FFFF; // set limit bits 15:0

	return descriptor;
}

void gdt_init(void)
{
	// 初始化 TSS
	tss.rsp0 = (__u64)(kernel_stack + sizeof(kernel_stack)); // 有效栈顶
	tss.rsp1 = 0;
	tss.rsp2 = 0;
	tss.ist1 = 0; // 可选：为 NMI/Double Fault 设置 IST 栈
	tss.iomap_base = sizeof(struct tss_entry); // 禁用 I/O 位图

	gdt[0] = 0; // null descriptor
	gdt[1] = create_descriptor(0, 0xfffff, GDT_CODE64_PL0);
	gdt[2] = create_descriptor(0, 0xfffff, GDT_DATA64_PL0);
	gdt[3] = create_descriptor(0, 0xfffff, GDT_CODE64_PL3);
	gdt[4] = create_descriptor(0, 0xfffff, GDT_DATA64_PL3);

	__u64 tss_base = (__u64)&tss;
	__u32 tss_limit = sizeof(struct tss_entry) - 1;

	// Low 64 bits
	gdt[5] = ((__u64)(tss_limit & 0xFFFF)) |
	         ((tss_base & 0xFFFFFFULL) << 16) | (0x89ULL << 40) |
	         ((__u64)(tss_limit & 0xF0000) << 48);

	// High 64 bits: base[63:32]
	gdt[6] = tss_base >> 32;

	g.limit = sizeof(gdt) * sizeof(__u64) - 1; // 注意：这里应是字节数！
	g.base = (__u64)gdt;

	asm volatile("lgdt %0" ::"m"(g));

	// 重载段寄存器
	asm volatile("movq %0, %%rax\n\t"
	             "movw %%ax, %%ds\n\t"
	             "movw %%ax, %%es\n\t"
	             "movw %%ax, %%fs\n\t"
	             "movw %%ax, %%gs\n\t"
	             "movw %%ax, %%ss"
	             :
	             : "r"((__u64)KDATA_SELECTOR)
	             : "rax");

	// 加载 TSS
	asm volatile("ltr %%ax" ::"a"(TSS_SELECTOR));
}

void update_tss_rsp0(__u64 rsp0) { tss.rsp0 = rsp0; }