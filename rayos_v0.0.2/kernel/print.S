    .section .text
    .global putc
putc:
    push %ebp
    mov %esp, %ebp
    mov 0x8(%ebp), %eax
    mov $0xb8000, %edi
    mov $print_position, %ecx
    mov (%ecx), %edx
    add %edx, %edi
    mov %al, (%edi)
    add $0x2, %edx
    mov %edx, (%ecx)
    shr $0x1, %edx
    push %edx
    call set_cursor
    xor %ecx, %ecx
    leave
    ret
 
    .global puts
puts:
    push %ebp
    mov %esp, %ebp
    mov 0x8(%ebp), %esi
lo:
    mov (%esi), %al
    cmp $0x0a, %al
    je puts_end
    push %eax
    call putc
    inc %esi
    jmp lo
puts_end:
    xor %ecx, %ecx
    leave
    ret

    .section .data
    .global print_position
print_position:
    .int 0x0