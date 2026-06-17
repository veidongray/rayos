#ifndef INT_H
#define INT_H

#include <stdint.h>

/* ============================================================
 *  x86 CPU 异常 (Exceptions) — INT 0x00 ~ 0x1F
 *  参考: Intel SDM Vol.3A, Table 6-1
 * ============================================================ */
#define X86_EXCEPT_DIVIDE_ERROR 0x00        /* #DE */
#define X86_EXCEPT_DEBUG 0x01               /* #DB */
#define X86_EXCEPT_NMI 0x02                 /* NMI */
#define X86_EXCEPT_BREAKPOINT 0x03          /* #BP */
#define X86_EXCEPT_OVERFLOW 0x04            /* #OF */
#define X86_EXCEPT_BOUND_RANGE 0x05         /* #BR */
#define X86_EXCEPT_INVALID_OPCODE 0x06      /* #UD */
#define X86_EXCEPT_DEVICE_NOT_AVAIL 0x07    /* #NM */
#define X86_EXCEPT_DOUBLE_FAULT 0x08        /* #DF (有错误码) */
#define X86_EXCEPT_COPROC_SEG_OVERRUN 0x09  /* 保留 */
#define X86_EXCEPT_INVALID_TSS 0x0A         /* #TS (有错误码) */
#define X86_EXCEPT_SEGMENT_NOT_PRESENT 0x0B /* #NP (有错误码) */
#define X86_EXCEPT_STACK_FAULT 0x0C         /* #SS (有错误码) */
#define X86_EXCEPT_GENERAL_PROTECTION 0x0D  /* #GP (有错误码) */
#define X86_EXCEPT_PAGE_FAULT 0x0E          /* #PF (有错误码) */
#define X86_EXCEPT_RESERVED_0F 0x0F         /* 保留 */
#define X86_EXCEPT_X87_FPE 0x10             /* #MF */
#define X86_EXCEPT_ALIGNMENT_CHECK 0x11     /* #AC (有错误码) */
#define X86_EXCEPT_MACHINE_CHECK 0x12       /* #MC */
#define X86_EXCEPT_SIMD_FPE 0x13            /* #XM/#XF */
#define X86_EXCEPT_VIRT_EXCEPTION 0x14      /* #VE (VT-x) */
#define X86_EXCEPT_CONTROL_PROTECTION 0x15  /* #CP (CET) */
/* 0x16 ~ 0x1F 保留 */

#define X86_EXCEPT_COUNT 32 /* 异常总数 */

/* ============================================================
 *  PIC / IOAPIC 硬件中断映射
 *  Linux 默认将 Master PIC 重映射到 0x20, Slave 到 0x28
 * ============================================================ */
#define X86_IRQ_BASE_MASTER 0x20
#define X86_IRQ_BASE_SLAVE 0x28

/* --- Master PIC (IRQ 0-7 → INT 0x20-0x27) --- */
#define X86_IRQ_TIMER (X86_IRQ_BASE_MASTER + 0) /* PIT / APIC Timer */
#define X86_IRQ_KEYBOARD (X86_IRQ_BASE_MASTER + 1)
#define X86_IRQ_CASCADE (X86_IRQ_BASE_MASTER + 2) /* 级联 Slave PIC */
#define X86_IRQ_COM2 (X86_IRQ_BASE_MASTER + 3)
#define X86_IRQ_COM1 (X86_IRQ_BASE_MASTER + 4)
#define X86_IRQ_LPT2 (X86_IRQ_BASE_MASTER + 5)
#define X86_IRQ_FLOPPY (X86_IRQ_BASE_MASTER + 6)
#define X86_IRQ_LPT1 (X86_IRQ_BASE_MASTER + 7)

/* --- Slave PIC (IRQ 8-15 → INT 0x28-0x2F) --- */
#define X86_IRQ_CMOS_RTC (X86_IRQ_BASE_SLAVE + 0)
#define X86_IRQ_MOUSE (X86_IRQ_BASE_SLAVE + 4)
#define X86_IRQ_FPU (X86_IRQ_BASE_SLAVE + 5)
#define X86_IRQ_PRIMARY_ATA (X86_IRQ_BASE_SLAVE + 6)
#define X86_IRQ_SECONDARY_ATA (X86_IRQ_BASE_SLAVE + 7)

/* ============================================================
 *  APIC 相关中断
 * ============================================================ */
#define X86_APIC_SPURIOUS_VECTOR 0xFF /* 伪中断向量 */
#define X86_APIC_TIMER_VECTOR 0xEF    /* Local APIC Timer */
#define X86_APIC_ERROR_VECTOR 0xEE    /* Local APIC Error */
#define X86_IPI_RESCHEDULE 0xFD       /* SMP 重新调度 (示例值) */
#define X86_IPI_CALL_FUNC 0xFC        /* SMP 函数调用 */
#define X86_IPI_TLB_SHOOTDOWN 0xFB    /* TLB 刷新 */

/* ============================================================
 *  Linux x86 系统调用 & 特殊软中断
 * ============================================================ */
#define X86_INT_SYSCALL 0x80     /* int 0x80 传统系统调用 */
#define X86_SYSENTER_VECTOR 0x80 /* SYSENTER 入口 (同向量号) */

/* ============================================================
 *  辅助宏
 * ============================================================ */

/** 判断一个向量号是否为 CPU 异常 */
#define X86_IS_EXCEPTION(vec) \
    ((unsigned)(vec) < X86_EXCEPT_COUNT)

/** 判断一个向量号是否属于硬件 IRQ 范围 */
#define X86_IS_HW_IRQ(vec) \
    ((unsigned)(vec) >= X86_IRQ_BASE_MASTER && (unsigned)(vec) <= 0x2F)

/** 获取异常是否有错误码 (位图法, bit=1 表示有错误码) */
#define X86_EXCEPT_HAS_ERROR_CODE_MASK 0x027D00U /* bits: 8,10,11,12,13,14,17 */
#define X86_EXCEPT_HAS_ERRCODE(vec) \
    (((X86_EXCEPT_HAS_ERROR_CODE_MASK >> (vec)) & 1U) != 0)

typedef struct
{
    uint16_t isr_low;   // The lower 16 bits of the ISR's address
    uint16_t kernel_cs; // The GDT segment selector that the CPU will load into CS before calling the ISR
    uint8_t ist;        // The IST in the TSS that the CPU will load into RSP; set to zero for now
    uint8_t attributes; // Type and attributes; see the IDT page
    uint16_t isr_mid;   // The higher 16 bits of the lower 32 bits of the ISR's address
    uint32_t isr_high;  // The higher 32 bits of the ISR's address
    uint32_t reserved;  // Set to zero
} __attribute__((packed)) idt_entry_t;

typedef struct
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

#define local_irq_enable() asm volatile("sti")
#define local_irq_disable() asm volatile("cli")

void int_init(void);
void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);

#endif // INT_H