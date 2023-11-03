    .section .text
    .global set_idt
set_idt:
    pushl %ebp
    movl %esp, %ebp
    movl $0x100, %ecx
    movl $0x20000, %edi
idt:
    movl $def_intservice, %eax
    movw %ax, (%edi)
    movw $0x8, 0x2(%edi)
    movw $0x8e00, 0x4(%edi)
    shrl $16, %eax
    movw %ax, 0x6(%edi)
    addl $0x8, %edi
    loop idt
    lidt idt_size
    ## set keyboard intservice proc
    movl $0x20108, %edi
    movl $keyboard_intservice, %eax
    movw %ax, (%edi)
    movw $0x8, 0x2(%edi)
    movw $0x8e00, 0x4(%edi)
    shrl $16, %eax
    movw %ax, 0x6(%edi)
    ## set timer intservice proc
    movl $0x20100, %edi
    movl $timer_intservice, %eax
    movw %ax, (%edi)
    movw $0x8, 0x2(%edi)
    movw $0x8e00, 0x4(%edi)
    shrl $16, %eax
    movw %ax, 0x6(%edi)
    leave
    ret

    .global def_intservice
def_intservice:
    cli
    iret

    .global keyboard_intservice
keyboard_intservice:
    cli
    call keyboard
    iret

    .global timer_intservice
timer_intservice:
    cli
    call timer_handle
    iret

    .section .data
idt_size:
    .word 0x7ff
    .int 0x00006000