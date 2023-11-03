    .section .text
    .global set_cursor
set_cursor:
    pushl %ebp
    movl %esp, %ebp
    movw 0x8(%ebp), %bx
    movb $0xe, %al
    movw $0x03d4, %dx
    out %al, %dx
    movw $0x03d5, %dx
    movb %bh, %al
    out %al, %dx
    movb $0xf, %al
    movw $0x03d4, %dx
    out %al, %dx
    movw $0x03d5, %dx
    movb %bl, %al
    out %al, %dx
    leave
    ret

    .global get_cursor
get_cursor:
    pushl %ebp
    movl %esp, %ebp
    movb $0xe, %al
    movw $0x03d4, %dx
    out %al, %dx
    movw $0x03d5, %dx
    in %dx, %al
    movb %al, %bh
    movb $0xf, %al
    movw $0x03d4, %dx
    out %al, %dx
    movw $0x03d5, %dx
    in %dx, %al
    movb %al, %bl
    movw %bx, %ax
    leave
    ret