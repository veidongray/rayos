.macro PUSH_ALL
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
.endm

.macro POP_ALL
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
.endm

.section .note.GNU-stack,"",@progbits
.section .text
.global isr_default_stub
isr_default_stub:
    jmp isr_default_stub

.extern isr_handler0
.global isr_stub0
isr_stub0:
    call isr_handler0
    iretq

.extern lapic_timer_handler
.global lapic_timer_stub
lapic_timer_stub:
    # 保存所有通用寄存器
    PUSH_ALL

    # 调用 C 处理函数
    call lapic_timer_handler

    # 恢复寄存器
    POP_ALL

    # 返回
    iretq