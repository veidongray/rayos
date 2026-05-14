.section .note.GNU-stack,"",@progbits
.section .text
.global load_page_directory
.type load_page_directory, @function
load_page_directory:
    push %ebp
    mov %esp, %ebp
    push %eax
    mov 12(%esp), %eax
    mov %eax, %cr3
    pop %eax
    leave
    ret

.global enable_paging
.type enable_paging, @function
enable_paging:
    push %ebp
    mov %esp, %ebp
    push %eax
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0
    pop %eax
    leave
    ret