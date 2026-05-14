#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

#define enable_irq()         \
	do                       \
	{                        \
		asm volatile("sti"); \
	} while (0)

#define disable_irq()        \
	do                       \
	{                        \
		asm volatile("cli"); \
	} while (0)

struct idt_entry
{
	uint16_t isr_low;	// The lower 16 bits of the ISR's address
	uint16_t kernel_cs; // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t reserved;	// Set to zero
	uint8_t attributes; // Type and attributes; see the IDT page
	uint16_t isr_high;	// The higher 16 bits of the ISR's address
} __attribute__((packed));

struct idtr
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

void set_idt_entry(uint8_t vector, void *isr, uint8_t flags);
void load_idt(struct idt_entry *idt, uint16_t size);
int idt_init(void);
int is_interrupts_enabled(void);

#endif // IRQ_H