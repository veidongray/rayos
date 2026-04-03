#ifndef PIC_H
#define PIC_H

#include <stdint.h>

// PIT 端口地址
#define PIT_COMMAND_PORT    0x43
#define PIT_CHANNEL0_PORT   0x40

// 工作模式：通道0、低字节/高字节分两次写、模式2（速率发生器）、二进制计数
#define PIT_CMD_CH0_MODE2   0x34

// 默认频率：100 Hz（每秒100次中断，即每10ms一次）
// PIT 输入时钟频率为 1.193182 MHz
#define TIMER_FREQ          100
#define PIT_DIVISOR         (1193182 / TIMER_FREQ)  // ≈ 11931

// IRQ0 对应的中断向量（需与 pic_remap(0x20, ...) 一致）
#define IRQ0_VECTOR         0x20

#define IRQ0_VECTOR  0x20  // PIT (Programmable Interval Timer)
#define IRQ1_VECTOR  0x21  // Keyboard
#define IRQ2_VECTOR  0x22  // Cascade (连接从 PIC)
#define IRQ3_VECTOR  0x23  // COM2 / COM4
#define IRQ4_VECTOR  0x24  // COM1 / COM3
#define IRQ5_VECTOR  0x25  // LPT2 / Sound Card
#define IRQ6_VECTOR  0x26  // Floppy Disk Controller
#define IRQ7_VECTOR  0x27  // LPT1 / Spurious

#define IRQ8_VECTOR  0x28  // RTC (Real-Time Clock)
#define IRQ9_VECTOR  0x29  // ACPI / Redirected IRQ2
#define IRQ10_VECTOR 0x2A  // Reserved / NIC
#define IRQ11_VECTOR 0x2B  // USB / SCSI
#define IRQ12_VECTOR 0x2C  // PS/2 Mouse
#define IRQ13_VECTOR 0x2D  // FPU / Coprocessor
#define IRQ14_VECTOR 0x2E  // Primary ATA Hard Disk
#define IRQ15_VECTOR 0x2F  // Secondary ATA Hard Disk

void pic_remap(int offset1, int offset2);
void pic_disable(void);
void pic_set_mask(uint8_t IRQline);
void pic_clear_mask(uint8_t IRQline);
void pic_sendEOI(uint8_t irq);
void timer_init(void);

#endif // PIC_H