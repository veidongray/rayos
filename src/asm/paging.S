.section .note.GNU-stack,"",@progbits

.section .text

.global load_page_directory
.type load_page_directory, @function
.code64
load_page_directory:
    # 加载指定的页目录表
    # 传入的值是页目录表的物理地址
    # 在 x86-64 System V ABI 中，第一个参数通过 %rdi 传递
    # void load_page_directory(uint64_t page_directory_base) // 64-bit physical address
    mov %rdi, %cr3          # 将页目录基址加载到 CR3
    ret

.global enable_paging
.type enable_paging, @function
.code64
enable_paging:
    # 启用分页机制
    # 设置 CR0 寄存器的 PG (Bit 31) 位
    mov %cr0, %rax          # 读取 CR0
    movabsq $0x80000000, %rcx
    orq %rcx, %rax    # 设置 PG 位 (Bit 31)
    mov %rax, %cr0          # 写回 CR0
    ret