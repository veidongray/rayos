.section .note.GNU-stack,"",@progbits
.section .text
.global switch_to
.type switch_to, @function
switch_to:
    movl task_esp, %eax # task_struct->esp offset
    movl 4(%esp), %edi  # task_struct pointer
    movl (%eax, %edi), %esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popl %edi
    popl %esi
    popl %ebp
    popl %ebx
    popl %edx
    popl %ecx
    popl %eax
    sti
    retl

.global context_switch
.type context_switch, @function
context_switch:
    pushl %eax
    pushl %ecx
    pushl %edx
    pushl %ebx
    pushl %ebp
    pushl %esi
    pushl %edi
    push %ds
    push %es
    push %fs
    push %gs
    movl task_esp, %eax # task_struct->esp offset
    movl 48(%esp), %esi   # current_task
    movl %esp, (%eax, %esi)   # current_task->esp = esp
resume:
    movl 52(%esp), %esi  # next_task
    movl (%eax, %esi), %esp   # next_task->esp = esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popl %edi
    popl %esi
    popl %ebp
    popl %ebx
    popl %edx
    popl %ecx
    popl %eax
    sti
    retl

.global	switch_to_user
.type switch_to_user, @function
switch_to_user:
    iretl

.section .data
.global	task_esp
task_esp:
    # task_struct->esp offset
    .long 0x0
