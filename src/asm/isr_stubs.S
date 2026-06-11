.include "asm/common.inc"

.section .note.GNU-stack,"",@progbits
.section .text
.global isr_default_stub
isr_default_stub:
    jmp isr_default_stub

.extern isr_handler0
.global isr_stub0
isr_stub0:
    PUSH_ALL

    call isr_handler0
    
    POP_ALL
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

.extern isr_syscall_handler
.global isr_syscall_stub
isr_syscall_stub:
    PUSH_ALL

    movq %rsp, %rdi
    call isr_syscall_handler

    POP_ALL
    iretq