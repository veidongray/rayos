    .section .text
    .global kb_intservice
kb_intservice:
    cli
    movw $0x60, %dx
    in %dx, %al
    pushl %eax
    call putc
    popl %eax
    movw $0x61, %dx
    out %al, %dx
    movb $0x20, %al
    movw $0x20, %dx
    out %al, %dx
    movw $0xa0, %dx
    out %al, %dx
    iret