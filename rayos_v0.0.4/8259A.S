    .section .text
    .global init_8259A
init_8259A:
    push %ebp
    movl %esp, %ebp

    movb $0x11, %al
    movw $0x20, %dx
    out %al, %dx
    call io_delay

    movw $0xa0, %dx
    out %al, %dx
    call io_delay

    movb $0x20, %al
    movw $0x21, %dx
    out %al, %dx
    call io_delay

    movb $0x28, %al
    movw $0xa1, %dx
    out %al, %dx
    call io_delay

    movb $0x4, %al
    movw $0x21, %dx
    out %al, %dx
    call io_delay

    movb $0x2, %al
    movw $0xa1, %dx
    out %al, %dx
    call io_delay

    movb $0x1, %al
    movw $0x21, %dx
    out %al, %dx
    call io_delay

    movw $0xa1, %dx
    out %al, %dx
    call io_delay

    movb $0xfd, %al
    movw $0x21, %dx
    out %al, %dx
    call io_delay

    movb $0xff, %al
    movw $0xa1, %dx
    out %al, %dx
    call io_delay

    leave
    ret

    .global io_delay
io_delay:
    nop
    nop
    nop
    nop
    ret