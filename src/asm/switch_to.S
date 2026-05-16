.section .note.GNU-stack,"",@progbits
.section .text
.global switch_to
.type switch_to, @function
switch_to:
    # 系统中第一个任务需要使用 switch_to 直接返回
    # 不能直接使用 context_switch
    # 在没有任何任务的情况下直接使用 context_switch
    # 会导致第一个任务的 esp 被配置成第一个任务返回前的 esp
    # 就会导致 esp 永远无法切换从而卡死在 start_kernel() 中
    mov task_esp, %eax # task_struct->esp offset
    mov 4(%esp), %edi  # task_struct pointer
    mov (%eax, %edi), %esp
    jmp .resume

.global context_switch
.type context_switch, @function
context_switch:
    # 必须保存好上下文的通用寄存器
    # 否则会导致任务切换后 ebp 等寄存器没有修改为新任务的值
    # 从而产生系统错误
    # 开发初期就是 ebp 没有保存切换值的原因
    # 造成了任务切换时的栈指针指向出错的问题
    push %eax
    push %ecx
    push %edx
    push %ebx
    push %ebp
    push %esi
    push %edi

    # 必须保存好各个段寄存器
    # 之前由于没有保存段寄存器
    # 导致在用户态执行movzbl等
    # 需要 段寄存器+变址寄存器 组合
    # 例如 ds+ebx 的情况下会导致系统错误
    # 因为段选择子没有被设置
    push %ds
    push %es
    push %fs
    push %gs
    mov task_esp, %eax # task_struct->esp offset
    mov 48(%esp), %esi   # current_task
    mov %esp, (%eax, %esi)   # current_task->esp = esp
    mov 52(%esp), %esi  # next_task
    mov (%eax, %esi), %esp   # next_task->esp = esp

    # 恢复任务的上下文
.resume:
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
    # 首次执行的用户程序需要先通过iret返回才能进入用户态
    # 但由于 context_switch 没有iret指令
    # 所以只能在用户栈的返回地址先放 switch_to_user 的地址执行 iret 指令
    # 在switch_to_user 的栈构建好真正的需要返回的用户栈
    # 详细代码在 task.c 的 utask_create()
    iret

.section .data
.global	task_esp
task_esp:
    # 这个变量用来定位 task_struct 结构体中 esp 成员变量的索引
    # 这样就能够保证不管未来 task_struct 结构体成员顺序怎么变化
    # 都能正确找到正确的 esp 成员
    # task_struct->esp offset
    .long 0x0
