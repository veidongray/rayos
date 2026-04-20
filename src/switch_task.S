.section .note.GNU-stack,"",@progbits
.section .text
.global switch_to_task
switch_to_task:
    movl 4(%esp), %edi
    movl %edi, %esp
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
    movl 32(%esp), %esi   # current_task
    movl %esp, (%esi)   # current_task->esp = esp
resume:
    movl 36(%esp), %esi  # next_task
    movl (%esi), %esp   # next_task->esp = esp
    popl %edi
    popl %esi
    popl %ebp
    popl %ebx
    popl %edx
    popl %ecx
    popl %eax
    sti
    retl