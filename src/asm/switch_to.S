.section .note.GNU-stack,"",@progbits
.section .text
.global switch_to
.type switch_to, @function
switch_to:
    mov task_esp, %eax # task_struct->esp offset
    mov 4(%esp), %edi  # task_struct pointer
    mov (%eax, %edi), %esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    pop %edi
    pop %esi
    pop %ebp
    pop %ebx
    pop %edx
    pop %ecx
    pop %eax
    sti
    ret

.global context_switch
.type context_switch, @function
context_switch:
    push %eax
    push %ecx
    push %edx
    push %ebx
    push %ebp
    push %esi
    push %edi
    push %ds
    push %es
    push %fs
    push %gs
    mov task_esp, %eax # task_struct->esp offset
    mov 48(%esp), %esi   # current_task
    mov %esp, (%eax, %esi)   # current_task->esp = esp
resume:
    mov 52(%esp), %esi  # next_task
    mov (%eax, %esi), %esp   # next_task->esp = esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    pop %edi
    pop %esi
    pop %ebp
    pop %ebx
    pop %edx
    pop %ecx
    pop %eax
    sti
    ret

.global	switch_to_user
.type switch_to_user, @function
switch_to_user:
    iret

.section .data
.global	task_esp
task_esp:
    # task_struct->esp offset
    .long 0x0
