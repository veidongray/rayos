.section .note.GNU-stack,"",@progbits
.section .text
.global default_isr
default_isr:
.default_loop:
    hlt
    jmp .default_loop

.global isr_pic_timer
isr_pic_timer:
    call timer_interrupt_handler
    iret

.global	isr_page_fault
isr_page_fault:
    call page_fault_handler
    iret

.global isr_double_fault
isr_double_fault:
    call double_fault_handler
    iret

.global isr_gp_fault
isr_gp_fault:
    pushl (%esp)        # Pass error_code to C function
    call gp_fault_handler
    addl $4, %esp
    iret

.global isr_syscall
isr_syscall:
    pushl %eax
    pushl %ecx
    pushl %edx
    pushl %ebx
    pushl %ebp
    pushl %esi
    pushl %edi
    pushl %eax
    call syscall_handler
    popl %eax
    popl %edi
    popl %esi
    popl %ebp
    popl %ebx
    popl %edx
    popl %ecx
    popl %eax
    iret