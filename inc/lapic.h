#ifndef LAPIC_H
#define LAPIC_H

#include <acpi.h>
#include <page.h>
#include <stdint.h>

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

// ICR 字段
#define ICR_INIT 0x00000500         // Delivery Mode = INIT
#define ICR_SIPI 0x00000600         // Delivery Mode = SIPI
#define ICR_LEVEL_ASSERT 0x00004000 // Level = Assert
#define ICR_PHYSICAL 0x00000000     // Destination Mode = Physical
#define ICR_FIXED 0x00000000        // Delivery Mode = Fixed (for INIT)

// 等待 IPI 发送完成
void lapic_wait_ipi(void);
// 发送 INIT IPI
void lapic_send_init(uint8_t apic_id);
// 发送 SIPI
void lapic_send_sipi(uint8_t apic_id, uint8_t vector);
void lapic_init(void);
void lapic_send_eoi(void);
void lapic_calibrate(void);
void ap_lapic_init(void);

#endif // APIC_H