.section .note.GNU-stack,"",@progbits
.section .text
.global load_page_directory
.type load_page_directory, @function
load_page_directory:
    # 加载指定的页目录表
    # 传入的值是页目录表的物理地址
    # 这个函数在C语言中的引用为
    # void load_page_directory(uint32_t *page_directory)
    # 调用后的栈为
    # 8 ------------------
    #   | page_directory |
    # 4 ------------------
    #   | return address |
    # 0 ------------------
    push %eax
    mov 8(%esp), %eax
    mov %eax, %cr3
    pop %eax
    ret

.global enable_paging
.type enable_paging, @function
enable_paging:
    push %eax
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0
    pop %eax
    ret