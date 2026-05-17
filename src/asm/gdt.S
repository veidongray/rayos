.section .note.GNU-stack,"",@progbits

.section .text
.global gdt_flush
.type gdt_flush, @function
.code64
gdt_flush:
    lgdt (%rdi)        # Load GDT from pointer in %rdi

    mov $0x10, %ax     # Kernel data segment selector
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs

    ret