.section .note.GNU-stack,"",@progbits
.section .multiboot2
.align 8
mboot:
    .long 0xe85250d6                # magic
    .long 0                         # architecture (0 = i386)
    .long end_mboot - mboot         # total size
    .long -(0xe85250d6 + 0 + (end_mboot - mboot))  # checksum

    # Request memory map information (required!)
    .word 6                         # type = MULTIBOOT_HEADER_TAG_MMAP
    .word 0                         # flags = 0
    .long 8                         # size = 8 bytes

    # End tag (mandatory)
    .word 0                         # type = 0 (end tag)
    .word 0                         # flags = 0
    .long 8                         # size = 8 bytes
end_mboot:

.set KERN_OFFSET, 0xC0000000

.code32
.align 4096
.section .text
.global	_start
.type _start, @function
_start:
    # save multiboot2 tag
    movl $(_boot_page_directory - KERN_OFFSET), %edi
    movl %eax, 4(%edi)
    movl %ebx, 8(%edi)

    # setup page
    movl $(_boot_page_directory - KERN_OFFSET), %eax
    movl $(_boot_page_table - KERN_OFFSET), %ebx
    movl %ebx, (%eax)
    orl $0x3, (%eax)
    movl %ebx, 768 * 4(%eax)
    orl $0x3, 768 * 4(%eax)
    movl %eax, 1023 * 4(%eax)
    orl $0x3, 1023 * 4(%eax)

    movl $1024, %ecx
    xorl %eax, %eax
    orl $0x3, %eax
    movl $(_boot_page_table - KERN_OFFSET), %edi
_fill_page_table:
    movl %eax, (%edi)
    addl $0x1000, %eax
    addl $0x4, %edi
    loop _fill_page_table

    # load page diretory
    movl $(_boot_page_directory - KERN_OFFSET), %eax
    movl %eax, %cr3

    # enable page
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0

    # setup stack
    movl $_kstack_top, %esp

    movl $(_boot_page_directory - KERN_OFFSET), %edi
    movl 4(%edi), %eax
    movl 8(%edi), %ebx
    # pass the address of the multiboot info structure (in %ebx) to main
    pushl %ebx
    # push the grub magic number (in %eax) to main
    pushl %eax
    # call to kernel
    call main
_main_loop:
    hlt
    jmp _main_loop
_start_end:

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


.section .data
.align 4096
.global _boot_page_directory
_boot_page_directory:
.rept 1024
    .long 0x2
.endr

.global _boot_page_table
_boot_page_table:
.rept 1024
    .long 0x2
.endr

.global _kstack
_kstack:
    .space 512 * 1024
_kstack_top:
