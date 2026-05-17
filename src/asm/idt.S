.section .note.GNU-stack,"",@progbits

.section .text

.global default_isr
default_isr:
    # 默认中断函数
    # 没有实现具体处理函数的中断都映射到这里
    # 这个函数什么都不做只是停机和循环
.default_loop:
    hlt
    jmp .default_loop

.global isr_pic_timer
isr_pic_timer:
    # 时钟中断处理函数
    # 必须保存当前的上下文
    pushfq                    # 保存 RFLAGS
    pushq %rax                # 保存所有 16 个通用寄存器
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rsp
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    call timer_interrupt_handler
    popq %r15                 # 恢复所有寄存器
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rsp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
    popfq                     # 恢复 RFLAGS
    iretq                     # 64 位中断返回

.global isr_page_fault
isr_page_fault:
    # Page Fault中断处理函数
    # 必须保存当前的上下文
    # 由于CPU自动保存了 Error Code 在栈中
    pushfq                    # 保存 RFLAGS
    pushq %rax                # 保存所有寄存器
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rsp
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # 栈布局（从高到低）：
    # [RIP of next instruction]
    # [CS (64-bit)]
    # [RFLAGS (64-bit)]
    # [Error Code (64-bit)] <- RSP points here after pushq 16 times
    # [R15 (64-bit)]
    # ...
    # [RAX (64-bit)]
    # 所以 Error Code 在 RSP + 8*17 = RSP + 136 处

    pushq 136(%rsp)           # 将 Error Code 推送到栈顶，作为 C 函数参数
    call page_fault_handler   # 调用 C 处理函数
    add $8, %rsp              # 清除 Error Code (8 字节)

    popq %r15                 # 恢复寄存器
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rsp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
    popfq                     # 恢复 RFLAGS
    iretq                     # 返回

.global isr_double_fault
isr_double_fault:
    pushfq
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rsp
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # Double fault 也有错误码
    pushq 136(%rsp)           # 取错误码
    call double_fault_handler
    add $8, %rsp              # 清除错误码

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rsp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
    popfq
    iretq

.global isr_gp_fault
isr_gp_fault:
    # 栈布局：
    # [RIP]
    # [CS]
    # [RFLAGS]
    # [Error Code] <- RSP
    # ...
    pushfq
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rsp
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    pushq 136(%rsp)           # 将 Error Code 推送至栈顶
    call gp_fault_handler
    add $8, %rsp              # 清除 Error Code

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rsp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
    popfq
    iretq

.global isr_syscall
isr_syscall:
    # Syscall handler (using x86-64 calling convention)
    pushfq                    # 保存 RFLAGS
    pushq %rax                # 保存所有寄存器
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rsp
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # 系统调用参数通常通过寄存器传递：
    # %rdi, %rsi, %rdx, %r10, %r8, %r9 (System V ABI)
    # 但如果你使用 int 指令，参数可能通过 %rax 传递
    # 这里假设 %rax 是系统调用号

    movq %rax, %rdi           # 将系统调用号作为第一个参数
    call syscall_handler

    # 恢复寄存器
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rsp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
    popfq                     # 恢复 RFLAGS

    iretq