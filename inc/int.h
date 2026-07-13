#ifndef INT_H
#define INT_H

#include <types.h>

/* CPU 异常 (INT 0x00-0x1F) */
#define X86_EXCEPT_DIVIDE_ERROR 0x00 /* #DE */
#define X86_EXCEPT_DEBUG 0x01        /* #DB */
#define X86_EXCEPT_NMI 0x02
#define X86_EXCEPT_BREAKPOINT 0x03       /* #BP */
#define X86_EXCEPT_OVERFLOW 0x04         /* #OF */
#define X86_EXCEPT_BOUND_RANGE 0x05      /* #BR */
#define X86_EXCEPT_INVALID_OPCODE 0x06   /* #UD */
#define X86_EXCEPT_DEVICE_NOT_AVAIL 0x07 /* #NM */
#define X86_EXCEPT_DOUBLE_FAULT 0x08     /* #DF, 有错误码 */
#define X86_EXCEPT_COPROC_SEG_OVERRUN 0x09
#define X86_EXCEPT_INVALID_TSS 0x0A         /* #TS, 有错误码 */
#define X86_EXCEPT_SEGMENT_NOT_PRESENT 0x0B /* #NP, 有错误码 */
#define X86_EXCEPT_STACK_FAULT 0x0C         /* #SS, 有错误码 */
#define X86_EXCEPT_GENERAL_PROTECTION 0x0D  /* #GP, 有错误码 */
#define X86_EXCEPT_PAGE_FAULT 0x0E          /* #PF, 有错误码 */
#define X86_EXCEPT_RESERVED_0F 0x0F
#define X86_EXCEPT_X87_FPE 0x10            /* #MF */
#define X86_EXCEPT_ALIGNMENT_CHECK 0x11    /* #AC, 有错误码 */
#define X86_EXCEPT_MACHINE_CHECK 0x12      /* #MC */
#define X86_EXCEPT_SIMD_FPE 0x13           /* #XM */
#define X86_EXCEPT_VIRT_EXCEPTION 0x14     /* #VE */
#define X86_EXCEPT_CONTROL_PROTECTION 0x15 /* #CP */
#define X86_EXCEPT_COUNT 32

/* PIC 硬件中断 (IRQ 0-15 → INT 0x20-0x2F) */
#define X86_IRQ_BASE_MASTER 0x20
#define X86_IRQ_BASE_SLAVE 0x28
#define X86_IRQ_TIMER (X86_IRQ_BASE_MASTER + 0)
#define X86_IRQ_KEYBOARD (X86_IRQ_BASE_MASTER + 1)
#define X86_IRQ_CASCADE (X86_IRQ_BASE_MASTER + 2)
#define X86_IRQ_COM2 (X86_IRQ_BASE_MASTER + 3)
#define X86_IRQ_COM1 (X86_IRQ_BASE_MASTER + 4)
#define X86_IRQ_LPT2 (X86_IRQ_BASE_MASTER + 5)
#define X86_IRQ_FLOPPY (X86_IRQ_BASE_MASTER + 6)
#define X86_IRQ_LPT1 (X86_IRQ_BASE_MASTER + 7)
#define X86_IRQ_CMOS_RTC (X86_IRQ_BASE_SLAVE + 0)
#define X86_IRQ_MOUSE (X86_IRQ_BASE_SLAVE + 4)
#define X86_IRQ_FPU (X86_IRQ_BASE_SLAVE + 5)
#define X86_IRQ_PRIMARY_ATA (X86_IRQ_BASE_SLAVE + 6)
#define X86_IRQ_SECONDARY_ATA (X86_IRQ_BASE_SLAVE + 7)

/* IOAPIC 中断向量 (GSI 重映射后的目标向量) */
#define X86_IOAPIC_VECTOR_BASE 0x30 /* IOAPIC 向量起始, 避开 PIC 范围 */
#define X86_IOAPIC_VECTOR_TIMER 0x30 /* IOAPIC Timer (替代 PIT) */
#define X86_IOAPIC_VECTOR_KEYBOARD 0x31
#define X86_IOAPIC_VECTOR_COM1 0x32
#define X86_IOAPIC_VECTOR_COM2 0x33
#define X86_IOAPIC_VECTOR_ATA_PRI 0x34
#define X86_IOAPIC_VECTOR_ATA_SEC 0x35
#define X86_IOAPIC_VECTOR_USB 0x36
#define X86_IOAPIC_VECTOR_SATA 0x37
#define X86_IOAPIC_VECTOR_PCIE_BASE 0x40 /* PCIe MSI/MSI-X 向量起始 */
#define X86_IOAPIC_VECTOR_MAX 0xEF /* IOAPIC 向量上限, 低于 APIC 专用向量 */

/* Local APIC 专用向量 */
#define X86_APIC_SPURIOUS_VECTOR 0xFF
#define X86_APIC_TIMER_VECTOR 0xEF
#define X86_APIC_ERROR_VECTOR 0xEE
#define X86_IPI_RESCHEDULE 0xFD
#define X86_IPI_CALL_FUNC 0xFC
#define X86_IPI_TLB_SHOOTDOWN 0xFB

/* 系统调用 */
#define X86_INT_SYSCALL 0x80
#define X86_SYSENTER_VECTOR 0x80

/* 辅助宏 */
#define X86_IS_EXCEPTION(vec) ((unsigned)(vec) < X86_EXCEPT_COUNT)

#define X86_IS_HW_IRQ(vec)                                                     \
	((unsigned)(vec) >= X86_IRQ_BASE_MASTER && (unsigned)(vec) <= 0x2F)

#define X86_EXCEPT_HAS_ERROR_CODE_MASK                                         \
	0x027D00U /* bit=1: 8,10,11,12,13,14,17 */
#define X86_EXCEPT_HAS_ERRCODE(vec)                                            \
	(((X86_EXCEPT_HAS_ERROR_CODE_MASK >> (vec)) & 1U) != 0)

/* ========== IDT 门类型 (Bits 11:8) ========== */
#define IDT_TYPE_INTERRUPT_GATE_64                                             \
	0x0E                       /* 64-bit Interrupt Gate (自动清除 IF) */
#define IDT_TYPE_TRAP_GATE_64 0x0F /* 64-bit Trap Gate (不改变 IF) */

/* ========== 特权级 DPL (Bits 14:13) ========== */
#define IDT_DPL_KERNEL (0 << 5) /* Ring 0: 仅内核态可触发 (int指令) */
#define IDT_DPL_USER (3 << 5) /* Ring 3: 用户态可通过 int 指令触发 */

/* ========== 存在位 (Bit 15) ========== */
#define IDT_PRESENT (1 << 7) /* 段/门有效标志 */

/* 内核硬件中断 & 异常处理 (最常用) */
#define IDT_FLAG_KERNEL_INT                                                    \
	(IDT_PRESENT | IDT_DPL_KERNEL | IDT_TYPE_INTERRUPT_GATE_64) /* 0x8E */

/* 内核陷阱/调试 (如 #BP, #OF) */
#define IDT_FLAG_KERNEL_TRAP                                                   \
	(IDT_PRESENT | IDT_DPL_KERNEL | IDT_TYPE_TRAP_GATE_64) /* 0x8F */

/* 用户态系统调用入口 (如 int 0x80 / syscall 兼容层) */
#define IDT_FLAG_USER_INT                                                      \
	(IDT_PRESENT | IDT_DPL_USER | IDT_TYPE_INTERRUPT_GATE_64) /* 0xEE */

/* 用户态陷阱 (极少用) */
#define IDT_FLAG_USER_TRAP                                                     \
	(IDT_PRESENT | IDT_DPL_USER | IDT_TYPE_TRAP_GATE_64) /* 0xEF */

typedef struct {
	__u16 isr_low;
	__u16 kernel_cs;
	__u8 ist;
	__u8 attributes;
	__u16 isr_mid;
	__u32 isr_high;
	__u32 reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct {
	__u16 limit;
	__u64 base;
} __attribute__((packed)) idtr_t;

#define local_irq_enable() asm volatile("sti")
#define local_irq_disable() asm volatile("cli")

void int_init(void);
void idt_set_descriptor(__u8 vector, void *isr, __u8 flags);

#endif /* INT_H */