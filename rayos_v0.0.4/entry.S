    .section .text
    .global entry
entry:
    movl $0x100, %ecx
    movl $0x6000, %edi
idt:
    movl $kb_intservice, %eax
    movw %ax, (%edi)
    movw $0x8, 0x2(%edi)
    movw $0x8e00, 0x4(%edi)
    shrl $16, %eax
    movw %ax, 0x6(%edi)
    addl $0x8, %edi
    loop idt
    lidt idt_size
    call init_8259A
    sti
    pushl $str1
    call puts
    popl %eax
    call main

    .section .data
idt_size:
    .word 0x7ff
    .int 0x00006000
str0:
    .string "                    --- Welcome to OS! ---                                      "
str1:
    .string "[%] Initialize 8259A...                                                         "