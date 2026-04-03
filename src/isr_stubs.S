#define ISR_STUB(num)             \
.global	isr_stub_##num          ; \
.type isr_stub_##num, @function ; \
isr_stub_##num:                 ; \
    push %ebp                   ; \
    mov %esp, %ebp              ; \
    push $num                   ; \
    call exception_handler      ; \
    leave                       ; \
    iret                        ;

#define ISR_TABLE_ENTRY(num) \
    .long isr_stub_##num;

.align 4
.section .note.GNU-stack,"",@progbits
.section .text
ISR_STUB(0)   # Divide by zero
ISR_STUB(1)   # Debug
ISR_STUB(2)   # Non-maskable interrupt
ISR_STUB(3)   # Breakpoint
ISR_STUB(4)   # Overflow
ISR_STUB(5)   # Bound range exceeded
ISR_STUB(6)   # Invalid opcode
ISR_STUB(7)   # Device not available
ISR_STUB(8)   # Double fault
ISR_STUB(9)   # Coprocessor segment overrun
ISR_STUB(10)  # Invalid TSS
ISR_STUB(11)  # Segment not present
ISR_STUB(12)  # Stack segment fault
ISR_STUB(13)  # General protection fault
ISR_STUB(14)  # Page fault
ISR_STUB(15)  # Reserved
ISR_STUB(16)  # x87 FPU floating-point error
ISR_STUB(17)  # Alignment check
ISR_STUB(18)  # Machine check
ISR_STUB(19)  # SIMD floating-point exception
ISR_STUB(20)  # Virtualization exception
ISR_STUB(21)  # Control protection exception
ISR_STUB(22)  # Reserved
ISR_STUB(23)  # Reserved
ISR_STUB(24)  # Reserved
ISR_STUB(25)  # Reserved
ISR_STUB(26)  # Reserved
ISR_STUB(27)  # Reserved
ISR_STUB(28)  # Reserved
ISR_STUB(29)  # Reserved
ISR_STUB(30)  # Reserved
ISR_STUB(31)  # Reserved

.global	isr_stub_table
isr_stub_table:
ISR_TABLE_ENTRY(0)
ISR_TABLE_ENTRY(1)
ISR_TABLE_ENTRY(2)
ISR_TABLE_ENTRY(3)
ISR_TABLE_ENTRY(4)
ISR_TABLE_ENTRY(5)
ISR_TABLE_ENTRY(6)
ISR_TABLE_ENTRY(7)
ISR_TABLE_ENTRY(8)
ISR_TABLE_ENTRY(9)
ISR_TABLE_ENTRY(10)
ISR_TABLE_ENTRY(11)
ISR_TABLE_ENTRY(12)
ISR_TABLE_ENTRY(13)
ISR_TABLE_ENTRY(14)
ISR_TABLE_ENTRY(15)
ISR_TABLE_ENTRY(16)
ISR_TABLE_ENTRY(17)
ISR_TABLE_ENTRY(18)
ISR_TABLE_ENTRY(19)
ISR_TABLE_ENTRY(20)
ISR_TABLE_ENTRY(21)
ISR_TABLE_ENTRY(22)
ISR_TABLE_ENTRY(23)
ISR_TABLE_ENTRY(24)
ISR_TABLE_ENTRY(25)
ISR_TABLE_ENTRY(26)
ISR_TABLE_ENTRY(27)
ISR_TABLE_ENTRY(28)
ISR_TABLE_ENTRY(29)
ISR_TABLE_ENTRY(30)
ISR_TABLE_ENTRY(31)