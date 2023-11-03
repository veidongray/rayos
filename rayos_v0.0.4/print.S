    .section .text
    .global putc
putc:
    pushl %ebp
    movl %esp, %ebp
    movb 0x8(%ebp), %al
    movl $0xb8000, %ebx
    addl vga_pos, %ebx
    movb %al, (%ebx)
    addl $0x2, vga_pos
    movl vga_pos, %eax
    shrl $0x1, %eax
    pushw %ax
    call set_cursor
    cmpw $4000, vga_pos
    jne putc_ret
    movl $0x0, vga_pos
    pushw vga_pos
    call set_cursor
putc_ret:
    leave
    ret

    .global puts
puts:
    pushl %ebp
    movl %esp, %ebp
    movl 0x8(%ebp), %esi
s0:
    pushl (%esi)
    call putc
    incl %esi
    cmpb $0x0, (%esi)
    jne s0
    leave
    ret

    .section .data
    .global vga_pos
vga_pos:
    .int 0x0