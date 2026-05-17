.section .note.GNU-stack,"",@progbits
.section .text
.code64

.global switch_to
.type switch_to, @function
switch_to:
    movq task_esp(%rip), %rax
    movq %rdi, %r8                     # rdi = next_task
    movq (%rax, %r8), %rsp
    jmp .resume

.global context_switch
.type context_switch, @function
context_switch:
    # 保存 callee-saved 寄存器
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # 保存段寄存器（推荐保留）
    movw %ds, %ax
    pushq %rax
    movw %es, %ax
    pushq %rax
    movw %fs, %ax
    pushq %rax
    movw %gs, %ax
    pushq %rax

    # 保存当前任务的 rsp
    movq task_esp(%rip), %rax
    movq %rsi, %r8                     # rsi = current
    movq %rsp, (%rax, %r8)

    # 切换到下一个任务
    movq %rdx, %r8                     # rdx = next
    movq (%rax, %r8), %rsp

.resume:
    # 恢复段寄存器
    popq %rax
    movw %ax, %gs
    popq %rax
    movw %ax, %fs
    popq %rax
    movw %ax, %es
    popq %rax
    movw %ax, %ds

    # 恢复 callee-saved 寄存器
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx

    sti
    ret

.global switch_to_user
.type switch_to_user, @function
switch_to_user:
    iretq

.section .data
.global task_esp
task_esp:
    .quad 0