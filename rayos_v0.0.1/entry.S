    .code32
    .section ".text", "ax"
    .global entry
entry:
    call main
    ret

    .global putc
putc:
    mov 0x8(%esp), %ecx
    mov $0xb8000, %edi
    add %ecx, %edi
    mov %al, (%edi)
    xor %eax, %eax
    ret