.section .note.GNU-stack,"",@progbits
.section .multiboot2
.align 8
_mboot:
    .long 0xe85250d6
    .long 0
    .long _end_mboot - _mboot
    .long -(0xe85250d6 + 0 + (_end_mboot - _mboot))

    .word 6, 0, 8
    .word 0, 0, 8
_end_mboot:

.set VIRT_OFFSET, 0xffff800000000000
.set NUM_PT, 16 # 32MB

# =====================================================================
.code32 
.section .text
.global _start
.type _start, @function
_start:
    cli
    cld
    
    # 保存 multiboot 信息（注意：此时仍在低端地址）
    movl %ebx, (_mboot_info - VIRT_OFFSET)
    movl %eax, (_mboot_magic - VIRT_OFFSET)

    # 关闭分页（GRUB 可能已开启）
    movl %cr0, %eax
    andl $0x7FFFFFFF, %eax
    movl %eax, %cr0

    # =====================================================================
    # 1. 创建 64-bit 页表（Higher-half 映射）
    # =====================================================================
    movl $(_boot_pml4 - VIRT_OFFSET), %edi
    xorl %eax, %eax
    movl $4096*4/4, %ecx          # 清空 16KB
    rep stosl

    # PML4[0]   → PDPT（低端恒等映射，方便过渡）
    # PML4[256] → PDPT（高端 higher-half）
    movl $(_boot_pml4 - VIRT_OFFSET), %edi
    movl $(_boot_pdpt - VIRT_OFFSET), %eax
    orl  $0x3, %eax

    movl %eax, (%edi)            # PML4[0]
    movl %eax, 256*8(%edi)        # PML4[256]
    movl %edi, 511*8(%edi)          # PML4[511]
    addl $0x3, 511*8(%edi)

    # PDPT[0] → PD（低端 0~1GB）
    movl $(_boot_pdpt - VIRT_OFFSET), %edi
    movl $(_boot_pd - VIRT_OFFSET), %eax
    orl  $0x3, %eax
    movl %eax, (%edi)            # PDPT[0]

    # PD[0] → PT0
    movl $(_boot_pd - VIRT_OFFSET), %edi
    movl $(_boot_pt - VIRT_OFFSET), %eax
    orl  $0x3, %eax
    movl $0x0, %edx
    movl $NUM_PT, %ecx

0:
    movl %eax, (%edi, %edx, 8)
    addl $0x1000, %eax
    incl %edx
    decl %ecx
    jnz 0b

    # Setup PT
    movl $(_boot_pt - VIRT_OFFSET), %edi
    movl $0x00000003, %eax          # Present + Writable
    movl $(512 * NUM_PT), %ecx

1:  movl %eax, (%edi)               # 低 32 位
    movl $0, 4(%edi)                # 高 32 位清零
    addl $0x1000, %eax
    addl $8, %edi
    decl %ecx
    jnz 1b                          # 使用局部标签 + jnz 代替 loop

    # =====================================================================
    # 2. 进入长模式
    # =====================================================================
    movl $(_boot_pml4 - VIRT_OFFSET), %eax
    movl %eax, %cr3

    movl %cr4, %eax
    orl  $(1 << 5), %eax          # PAE
    movl %eax, %cr4

    movl $0xC0000080, %ecx
    rdmsr
    orl  $(1 << 8), %eax          # LME
    wrmsr

    movl %cr0, %eax
    orl  $0x80000000, %eax        # PG
    movl %eax, %cr0

    # 加载 64-bit GDT
    movl $(_gdt64_ptr - VIRT_OFFSET), %eax
    lgdt (%eax)

    # 远跳转进入长模式
    pushl $0x0008                    # 代码段选择子
    pushl $_start64 - VIRT_OFFSET    # 64位目标地址（低32位足够，因为高位是0xffffffff，但这里用pushl）
    lret                             # 执行远返回

# =====================================================================
.code64
_start64:
    movabsq $0x10, %rax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw %ax, %fs
    movw %ax, %gs

    # 现在可以安全使用高端地址了
    movabsq $_kstack_top, %rsp
    movabsq $start_kernel, %rax
    call *%rax

_loop:
    hlt
    jmp _loop

# =====================================================================
.align 4096
.section .data
_boot_pml4: .space 4096
_boot_pdpt: .space 4096
_boot_pd:   .space 4096
_boot_pt:   .space 4096 * NUM_PT

.align 8
_gdt64:
    .quad 0x0000000000000000
    .quad 0x00af9a000000ffff   # 64-bit code
    .quad 0x00cf92000000ffff   # 64-bit data
_gdt64_end:

_gdt64_ptr:
    .word _gdt64_end - _gdt64 - 1
    .long _gdt64 - VIRT_OFFSET     # 32位阶段用物理地址

.global _mboot_info
.global _mboot_magic
_mboot_info: .long 0
_mboot_magic: .long 0

.align 4096
_kstack: .space 128 * 1024
_kstack_top: