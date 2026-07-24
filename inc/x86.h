#ifndef X86_H
#define X86_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val)
{
	__asm__ volatile("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
	/* There's an outb %al, $imm8 encoding, for compile-time constant port
	 * numbers that fit in 8b. (N constraint). Wider immediate constants
	 * would be truncated at assemble-time (e.g. "i" constraint). The  outb
	 * %al, %dx encoding is the only option for all other cases. %1 expands
	 * to %dx because port  is a uint16_t.  %w1 could be used if we had the
	 * port number a wider C type */
}

static inline uint8_t inb(uint16_t port)
{
	uint8_t ret;
	__asm__ volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
	return ret;
}

static inline void outl(uint16_t port, uint32_t val)
{
	__asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port)
{
	uint32_t val;
	__asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
	return val;
}

static inline void io_wait(void) { outb(0x80, 0); }

static inline void write_cr3(uint64_t pml4addr)
{
	asm volatile("movq %0, %%rax\r\n"
	             "movq %%rax, %%cr3\r\n"
	             :
	             : "r"(pml4addr)
	             : "rax");
}

static inline uint64_t read_cr3(void)
{
	uint64_t retval;

	asm volatile("movq %%cr3, %0" : "=r"(retval) : : "rax");
	return retval;
}

static inline void hlt(void) { asm volatile("hlt"); }

#endif // X86_H