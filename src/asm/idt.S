.section .note.GNU-stack,"",@progbits
.section .text
.global default_isr
default_isr:
.default_loop:
    hlt
    jmp .default_loop

.global isr_pic_timer
isr_pic_timer:
    pusha
    call timer_interrupt_handler
    popa
    iret

.global	isr_page_fault
isr_page_fault:
    call page_fault_handler
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
    push %eax
    call syscall_handler
    add $0x4, %esp
    iret