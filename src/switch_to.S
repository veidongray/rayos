.align	4096
.section .text
.global	switch_to_task
.type switch_to_task, @function
switch_to_task:
    mov 4(%esp), %esi   # current_task
    mov (%esi), %esp
    sti
    ret


.global context_switch
.type context_switch, @function
context_switch:
    mov 4(%esp), %esi   # current_task
    mov %esp, (%esi)   # current_task->esp = esp
resume:
    mov 8(%esp), %esi  # next_task
    mov (%esi), %esp   # next_task->esp = esp
    sti
    ret