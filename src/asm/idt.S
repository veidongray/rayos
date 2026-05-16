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
    # 开发初期没有保存好上下文
    # 导致莫名其妙的 Page Fault 出现
    pushf
    pusha
    call timer_interrupt_handler
    popa
    popf
    iret

.global	isr_page_fault
isr_page_fault:
    # Page Fault中断处理函数
    # 必须保存当前的上下文
    # 由于CPU自动保存了 Error Code 在栈中
    # 所以在 call 之前需要再从栈中取出 Error Code 放在栈顶处
    pushf
    pusha
    pushl 36(%esp)
    call page_fault_handler
    add $4, %esp
    popa
    popf
    iret

.global isr_double_fault
isr_double_fault:
    pusha
    call double_fault_handler
    popa
    iret

.global isr_gp_fault
isr_gp_fault:
    push (%esp)        # Pass error_code to C function
    call gp_fault_handler
    add $4, %esp
    iret

.global isr_syscall
isr_syscall:
    push %esp
    push %ecx
    push %edx
    push %ebx
    push %ebp
    push %esi
    push %edi

    # 因为会传入参数
    # 这里一样在 call 之前需要将参数放到栈顶
    push %eax
    call syscall_handler
    add $0x4, %esp
    
    pop %edi
    pop %esi
    pop %ebp
    pop %ebx
    pop %edx
    pop %ecx
    pop %esp
    iret