.section .text
.global gdt_flush
.type gdt_flush, @function
gdt_flush:
    push %ebp
    mov %esp, %ebp
    mov 8(%ebp), %eax      # Load GDT pointer (from C argument)
    lgdt (%eax)

    ljmp $0x08, $.reload     # Far jump to reload CS

.reload:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs
    leave
    ret