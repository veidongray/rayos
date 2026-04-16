.section .note.GNU-stack,"",@progbits
.section .text
.global load_page_directory
.type load_page_directory, @function
load_page_directory:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%esp), %eax
    movl %eax, %cr3
    leavel
    retl

.global enable_paging
.type enable_paging, @function
enable_paging:
    pushl %ebp
    movl %esp, %ebp
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0
    leavel
    retl