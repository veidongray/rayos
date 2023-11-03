    .section .text
    .global set_cursor
set_cursor:
    push %ebp
    mov %esp, %ebp
    mov 0x8(%ebp), %bx
    mov $0xe, %al
    mov $0x03d4, %dx
    out %al, %dx
    mov $0x03d5, %dx
    mov %bh, %al
    out %al, %dx
    mov $0xf, %al
    mov $0x03d4, %dx
    out %al, %dx
    mov $0x03d5, %dx
    mov %bl, %al
    out %al, %dx
    leave
    ret

    .global get_cursor
get_cursor:
    push %ebp
    mov %esp, %ebp
    mov $0xe, %al
    mov $0x03d4, %dx
    out %al, %dx
    mov $0x03d5, %dx
    in %dx, %al
    mov %al, %bh
    mov $0xf, %al
    mov $0x03d4, %dx
    out %al, %dx
    mov $0x03d5, %dx
    in %dx, %al
    mov %al, %bl
    mov %bx, %ax
    leave
    ret