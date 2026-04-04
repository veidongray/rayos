.section .note.GNU-stack,"",@progbits
.section .multiboot2
.align 8
mboot:
    .long 0xe85250d6          # magic
    .long 0                   # architecture (0 = i386)
    .long end_mboot - mboot   # total size
    .long -(0xe85250d6 + 0 + (end_mboot - mboot))  # checksum

    # End tag (mandatory!)
    .word 0                   # type = 0 (end tag)
    .word 0                   # flags = 0
    .long 8                   # size = 8 bytes
end_mboot:

.code32		# to 32-bit mode
.section .text
.global _start
.type _start, @function
_start:
    # Set up the stack
    mov $stack_top, %esp
    # Never return
    call main
    hlt

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

.global load_page_directory
.type load_page_directory, @function
load_page_directory:
    push %ebp
    mov %esp, %ebp
    mov 8(%esp), %eax
    mov %eax, %cr3
    mov %ebp, %esp
    pop %ebp
    ret

.global enable_paging
.type enable_paging, @function
enable_paging:
    push %ebp
    mov %esp, %ebp
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0
    mov %ebp, %esp
    pop %ebp
    ret

.section .bss
.align 16
stack:
    .space 1 * 1024 * 1024  # 1MB stack
stack_top:
