.align	4096
.section .text
.global	switch_to_task
.type switch_to_task, @function
switch_to_task:
    mov 4(%esp), %esi   # current_task
    mov (%esi), %esp
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
    mov 32(%esp), %esi   # current_task
    mov %esp, (%esi)   # current_task->esp = esp
resume:
    mov 36(%esp), %esi  # next_task
    mov (%esi), %esp   # next_task->esp = esp
    pop %edi
    pop %esi
    pop %ebp
    pop %ebx
    pop %edx
    pop %ecx
    pop %eax
    sti
    ret