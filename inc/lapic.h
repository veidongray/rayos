#ifndef LAPIC_H
#define LAPIC_H

#include <page.h>
#include <types.h>

#define LAPIC_BASE 0xFEE00000

#define LAPIC_ID 0x0020        // ID 寄存器
#define LAPIC_VER 0x0030       // 版本寄存器
#define LAPIC_TPR 0x0080       // 任务优先级
#define LAPIC_EOI 0x00B0       // 中断结束
#define LAPIC_SVR 0x00F0       // 伪中断向量
#define LAPIC_ESR 0x0280       // 错误状态
#define LAPIC_LVT_TMR 0x0320   // LVT 定时器
#define LAPIC_LVT_PERF 0x0340  // 性能计数器
#define LAPIC_LVT_LINT0 0x0350 // LINT0 (通常连接 PIC)
#define LAPIC_LVT_LINT1 0x0360 // LINT1 (通常连接 NMI)
#define LAPIC_LVT_ERR 0x0370   // 错误
#define LAPIC_TICFG 0x03E0     // 定时器分频
#define LAPIC_TDCR 0x03E0      // 与上面相同
#define LAPIC_TIC 0x0380       // 初始计数
#define LAPIC_TCC 0x0390       // 当前计数

// 读写内存映射寄存器的辅助函数
static inline uint32_t lapic_read(uint32_t reg)
{
    // 先将 64 位基地址与 32 位偏移量相加，得到完整的 64 位虚拟地址，再强转为指针
    return *(volatile uint32_t *)((uintptr_t)(KERNEL_BASE + LAPIC_BASE + reg));
}

static inline void lapic_write(uint32_t reg, uint32_t data)
{
    *(volatile uint32_t *)((uintptr_t)(KERNEL_BASE + LAPIC_BASE + reg)) = data;
}

void lapic_init(void);
void lapic_send_eoi(void);
void lapic_calibrate(void);

#endif // APIC_H