.include "asm/common.inc"

.section .note.GNU-stack,"",@progbits
.section .text
.global switch_to
switch_to:
    movq %rdi, %rsp
    jmp resume_context

.global context_switch
context_switch:
    PUSH_ALL

    movq %rsp, (%rdi)
    movq (%rsi), %rsp
resume_context:
    POP_ALL
    sti
    retq

.global switch_to_user
switch_to_user:
    iretq