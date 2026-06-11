#include <int.h>
#include <pic.h>
#include <vfs.h>
#include <task.h>
#include <lapic.h>
#include <printk.h>

__attribute__((aligned(4096))) static idtr_t idtr;
__attribute__((aligned(4096))) static idt_entry_t idt[256]; // Create an array of IDT entries; aligned for performance

void isr_handler0(void);

extern void isr_stub0(void);
extern void isr_default_stub(void);
extern void lapic_timer_stub(void);
extern void isr_syscall_stub(void);

void int_init(void)
{
    int vector;

    for (vector = 0; vector < 256; vector++)
    {
        idt_set_descriptor(vector, isr_default_stub, 0x8E);
    }
    idt_set_descriptor(0, isr_stub0, 0x8E);
    idt_set_descriptor(32, lapic_timer_stub, 0x8E);
    idt_set_descriptor(0x80, isr_syscall_stub, 0xEE);

    idtr.base = (uint64_t)idt;
    idtr.limit = sizeof(idt) - 1;
    asm volatile("lidt %0" : : "m"(idtr));

    pic_remap(0x20, 0x28);
    pic_timer_init();
}

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags)
{
    idt_entry_t *descriptor = &idt[vector];

    descriptor->isr_low = (uint64_t)isr & 0xFFFF;
    descriptor->kernel_cs = 0x08;
    descriptor->ist = 0;
    descriptor->attributes = flags;
    descriptor->isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->isr_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;
}

void isr_handler0(void)
{
    lapic_send_eoi();
    // Do nothing
}

void lapic_timer_handler(void)
{
    lapic_send_eoi();
    scheduler();
}

int isr_syscall_handler(struct context *ctx)
{
    uint64_t nr = ctx->rax;

    switch (nr)
    {
    case SYS_OPEN:
        ctx->rax = sys_open(ctx->rdi);
        break;

    case SYS_CLOSE:
        ctx->rax = sys_close(ctx->rdi);
        break;

    case SYS_READ:
        ctx->rax = sys_read(ctx->rdi, ctx->rsi, ctx->rdx);
        break;

    case SYS_WRITE:
        ctx->rax = sys_write(ctx->rdi, ctx->rsi, ctx->rdx);
        break;

    case SYS_CREATE:
        ctx->rax = sys_create(ctx->rdi);
        break;

    default:
        ctx->rax = -1;
        break;
    }
    return 0;
}