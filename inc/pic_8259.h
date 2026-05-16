#ifndef PIC_8259_H
#define PIC_8259_H

#include <stdint.h>

#define PIT_COMMAND_PORT 0x43
#define PIT_CHANNEL0_PORT 0x40

// Operating modes: Channel 0, low byte/high byte written in two steps,
// Mode 2 (rate generator), binary counter.
#define PIT_CMD_CH0_MODE2 0x34

// Default frequency: 100 Hz (100 interrupts per second, i.e., once every 10ms)
// PIT input clock frequency is 1.193182 MHz
#define TIMER_FREQ 100
#define PIT_DIVISOR (1193182 / TIMER_FREQ) // ≈ 11931

// The interrupt vector corresponding to IRQ0 (must be consistent with pic_remap(0x20, ...))
#define IRQ0_VECTOR 0x20

#define IRQ0_VECTOR 0x20 // PIT (Programmable Interval Timer)
#define IRQ1_VECTOR 0x21 // Keyboard
#define IRQ2_VECTOR 0x22 // Cascade (connect slave PIC)
#define IRQ3_VECTOR 0x23 // COM2 / COM4
#define IRQ4_VECTOR 0x24 // COM1 / COM3
#define IRQ5_VECTOR 0x25 // LPT2 / Sound Card
#define IRQ6_VECTOR 0x26 // Floppy Disk Controller
#define IRQ7_VECTOR 0x27 // LPT1 / Spurious

#define IRQ8_VECTOR 0x28  // RTC (Real-Time Clock)
#define IRQ9_VECTOR 0x29  // ACPI / Redirected IRQ2
#define IRQ10_VECTOR 0x2A // Reserved / NIC
#define IRQ11_VECTOR 0x2B // USB / SCSI
#define IRQ12_VECTOR 0x2C // PS/2 Mouse
#define IRQ13_VECTOR 0x2D // FPU / Coprocessor
#define IRQ14_VECTOR 0x2E // Primary ATA Hard Disk
#define IRQ15_VECTOR 0x2F // Secondary ATA Hard Disk

void pic_remap(int offset1, int offset2);
void pic_disable(void);
void pic_set_mask(uint8_t IRQline);
void pic_clear_mask(uint8_t IRQline);
void pic_sendEOI(uint8_t irq);
void pic_timer_init(void);
void pit_prepare_sleep_10ms(void);
void pit_wait_10ms(void);

#endif // PIC_8259_H