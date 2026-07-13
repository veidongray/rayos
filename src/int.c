#include <gdt.h>
#include <int.h>
#include <lapic.h>
#include <pic.h>
#include <printk.h>
#include <syscalls.h>
#include <task.h>
#include <types.h>
#include <uart.h>
#include <vfs.h>

__attribute__((aligned(4096))) static idtr_t __idtr;
__attribute__((aligned(4096))) static idt_entry_t
        __idt[256]; // Create an array of IDT entries; aligned for performance

/**
 * From isr_stubs.S
 */
extern void isr_divide_error_stub(void);
extern void isr_default_stub(void);
extern void lapic_timer_stub(void);
extern void isr_syscall_stub(void);
extern void isr_page_fault_stub(void);
extern void isr_com1_stub(void);

void int_init(void)
{
	int vector;

	for (vector = 0; vector < 256; vector++) {
		idt_set_descriptor(vector, isr_default_stub,
		                   IDT_FLAG_KERNEL_INT);
	}
	idt_set_descriptor(X86_EXCEPT_DIVIDE_ERROR, isr_divide_error_stub,
	                   IDT_FLAG_KERNEL_INT);
	idt_set_descriptor(X86_EXCEPT_PAGE_FAULT, isr_page_fault_stub,
	                   IDT_FLAG_KERNEL_INT);
	idt_set_descriptor(X86_APIC_TIMER_VECTOR, lapic_timer_stub,
	                   IDT_FLAG_KERNEL_INT);
	idt_set_descriptor(X86_INT_SYSCALL, isr_syscall_stub,
	                   IDT_FLAG_USER_INT);
	idt_set_descriptor(X86_IRQ_COM1, isr_com1_stub, IDT_FLAG_KERNEL_INT);

	__idtr.base = (__u64)__idt;
	__idtr.limit = sizeof(__idt) - 1;
	asm volatile("lidt %0" : : "m"(__idtr));

	pic_remap(0x20, 0x28);
	pic_timer_init();
}

void idt_set_descriptor(__u8 __vector, void *__isr, __u8 __flags)
{
	idt_entry_t *descriptor = &__idt[__vector];

	descriptor->isr_low = (__u64)__isr & 0xFFFF;
	descriptor->kernel_cs = KCODE_SELECTOR;
	descriptor->ist = 0;
	descriptor->attributes = __flags;
	descriptor->isr_mid = ((__u64)__isr >> 16) & 0xFFFF;
	descriptor->isr_high = ((__u64)__isr >> 32) & 0xFFFFFFFF;
	descriptor->reserved = 0;
}

void isr_divide_error_handler(void)
{
	lapic_send_eoi();
	// Do nothing
}

void lapic_timer_handler(void)
{
	scheduler();
	lapic_send_eoi();
}

void isr_page_fault_handler(__u64 __error, __u64 *__pagefault_addr)
{
	printk("Page fault! %#llx, error code %#llx", __pagefault_addr,
	       __error);
	asm volatile("hlt");
}

void isr_com1_handler(void)
{
	uart_isr_receive();
	lapic_send_eoi();
}

int isr_syscall_handler(struct context *ctx)
{
	__u64 nr = ctx->rax;

	switch (nr) {
	case SYS_OPEN:
		ctx->rax = sys_open((const char *)ctx->rdi, (__mode_t)ctx->rsi);
		break;

	case SYS_CLOSE:
		ctx->rax = sys_close((int)ctx->rdi);
		break;

	case SYS_READ:
		ctx->rax = sys_read((int)ctx->rdi, (char *)ctx->rsi,
		                    (size_t)ctx->rdx);
		break;

	case SYS_WRITE:
		ctx->rax = sys_write((int)ctx->rdi, (const char *)ctx->rsi,
		                     (size_t)ctx->rdx);
		break;

	case SYS_CREATE:
		ctx->rax = sys_create((const char *)ctx->rdi);
		break;

	default:
		ctx->rax = -1;
		break;
	}

	lapic_send_eoi();
	return 0;
}