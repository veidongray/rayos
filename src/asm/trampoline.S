.set AP_BASE, 0x8000

.extern ap_start

.section .note.GNU-stack,"",@progbits

.section .text.trampoline, "ax"
.code16
.align 16
.global ap_trampoline_start
ap_trampoline_start:
    cli
    cld

    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs

    # 加载 32-bit 临时 GDT（仅用于进入保护模式）
    lgdt   (ap_gdt32_ptr - ap_trampoline_start + AP_BASE)

    # CR0.PE = 1 → 进入保护模式
    movl    %cr0, %eax
    orl     $1, %eax
    movl    %eax, %cr0

    # 远跳转刷新 CS，进入 32-bit 段
    ljmp    $0x08, $(ap_protected - ap_trampoline_start + AP_BASE)

# =====================================================================
# 32-bit 保护模式
# =====================================================================
.code32
ap_protected:
    movw    $0x10, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    movw    %ax, %fs
    movw    %ax, %gs

    # CR3
    movl    (ap_cr3 - ap_trampoline_start + AP_BASE), %eax
    movl    %eax, %cr3

    # PAE
    movl    %cr4, %eax
    orl     $(1 << 5), %eax
    movl    %eax, %cr4

    # LME
    movl    $0xC0000080, %ecx
    rdmsr
    orl     $(1 << 8), %eax
    wrmsr

    # PG
    movl    %cr0, %eax
    orl     $0x80000000, %eax
    movl    %eax, %cr0

    # 加载 64-bit GDT（32-bit 模式下直接用绝对偏移即可）
    lgdtl   (ap_gdt64_ptr - ap_trampoline_start + AP_BASE)

    ljmp    $0x08, $(ap_long - ap_trampoline_start + AP_BASE)

# =====================================================================
# 64-bit 长模式
# =====================================================================
.code64
ap_long:
    # 设置数据段
    movw    $0x10, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    movw    %ax, %fs
    movw    %ax, %gs

    # 获取 Local APIC ID
    mov $1, %eax
    cpuid
    shr $24, %ebx           # EBX[31:24] = Initial APIC ID
    mov %bl, %bl            # BL = APIC ID

    // setup stack
    mov $(ap_stack_array - ap_trampoline_start + AP_BASE), %rsi
    mov (%rsi, %rbx, 8), %rsp

    movabsq $(ap_start), %rax
    jmp *%rax
l:
    hlt
    jmp l

.align 8
ap_gdt32:
    .quad 0x0000000000000000        # null
    .quad 0x00cf9a000000ffff        # 32-bit code, base=0, limit=4GB
    .quad 0x00cf92000000ffff        # 32-bit data
ap_gdt32_end:

ap_gdt32_ptr:
    .word ap_gdt32_end - ap_gdt32 - 1
    .long ap_gdt32 - ap_trampoline_start + AP_BASE   # 相对于 trampoline 基址的偏移

.align 8
ap_gdt64:
    .quad 0x0000000000000000
    .quad 0x00af9a000000ffff        # 64-bit code
    .quad 0x00cf92000000ffff        # 64-bit data
ap_gdt64_end:

ap_gdt64_ptr:
    .word ap_gdt64_end - ap_gdt64 - 1
    .long ap_gdt64 - ap_trampoline_start + AP_BASE   # 物理偏移（32-bit lgdt 用）

.align 8
.global ap_cr3
ap_cr3: .quad 0

.align 8
.global ap_stack_array
ap_stack_array: .quad MAX_CPUS * 8

.global ap_trampoline_end
ap_trampoline_end: