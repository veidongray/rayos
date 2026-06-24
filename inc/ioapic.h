#ifndef IO_APIC_H
#define IO_APIC_H

/* IOAPIC 内部寄存器 (通过 IOREGSEL/IOWIN 间接访问) */
#define IOAPIC_REG_ID 0x00     /* ID 寄存器 */
#define IOAPIC_REG_VER 0x01    /* 版本 & 最大重定向条目数 */
#define IOAPIC_REG_ARB 0x02    /* 仲裁 ID */
#define IOAPIC_REG_REDTBL 0x10 /* 重定向表基址 (每条目占2个寄存器) */

/* 重定向表条目 (RTE) 位定义 */
#define RTE_VECTOR_MASK 0xFF             /* [7:0]   中断向量号 */
#define RTE_DELIVERY_FIXED (0ULL << 8)   /* [10:8]  固定投递模式 */
#define RTE_DELIVERY_LOWEST (1ULL << 8)  /* 最低优先级投递 */
#define RTE_DEST_PHYSICAL (0ULL << 11)   /* [11]    物理目标模式 */
#define RTE_ACTIVE_HIGH (0ULL << 13)     /* [13]    高电平触发 */
#define RTE_EDGE_TRIGGERED (0ULL << 15)  /* [15]    边沿触发 */
#define RTE_LEVEL_TRIGGERED (1ULL << 15) /* 电平触发 */
#define RTE_MASKED (1ULL << 16)          /* [16]    屏蔽中断 */
#define RTE_DEST_SHIFT 56                /* [63:56] 目标 APIC ID */

void ioapic_init(void);
void ioapic_enable_irq(__u8 irq, __u8 vector, __u8 dest_apic_id, __u64 flags);
void ioapic_disable_irq(__u8 irq);

#endif // IO_APIC_H