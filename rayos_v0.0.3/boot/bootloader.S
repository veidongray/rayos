#include "boot.h"
    .code16
    .section    .text
    .global     start
start:
    ## close interrupt
    cli

    ## init regs
    xor     %eax, %eax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %ss
    mov     %ax, %fs
    mov     %ax, %gs

    ## open A20 address line
seta20_1:
    in      $0x64, %al
    test    $0x2, %al
    jnz     seta20_1

    mov     $0xd1, %al
    out     %al, $0x64

seta20_2:
    in      $0x64, %al
    test    $0x2, %al
    jnz     seta20_2

    mov     $0xdf, %al
    out     %al, $0x60

    ## load gdt base to gdtr
    lgdt    gdtdesc

    ## open pm
    mov     %cr0, %eax
    or      $0x1, %eax
    mov     %eax, %cr0

    ## flush cpu stream to new 32bit code
    ljmp    $KERNEL_GDT_CODESEG, $start32
    .code32
start32:
    mov     $KERNEL_GDT_DATASEG, %eax
    mov     %eax, %ds
    mov     %eax, %es
    mov     %eax, %ss
    xor     %eax, %eax
    mov     %eax, %fs
    mov     %eax, %gs

    ## load kernel
read_kernel:
    mov     $LOAD_SEC_LOGIC, %ebx           # sector logic number => 1
    mov     $LOAD_SEC_NUM, %ecx             # read sectors => 8kb
    mov     $KERNEL_LOAD_ADDR, %edi                 # dest address => 1MB

    mov     %cl, %al
    mov     $0x1f2, %dx
    out     %al, %dx

    mov     %bl, %al
    mov     $0x1f3, %dx
    out     %al, %dx

    shr     $8, %ebx
    mov     %bl, %al
    mov     $0x1f4, %dx
    out     %al, %dx

    shr     $8, %ebx
    mov     %bl, %al
    mov     $0x1f5, %dx
    out     %al, %dx

    shr     $8, %ebx
    and     $0x0f, %bl
    or      $0xe0, %bl
    mov     %bl, %al
    mov     $0x1f6, %dx
    out     %al, %dx

    mov     $0x20, %al
    mov     $0x1f7, %dx
    out     %al, %dx

wait_hhd:
    in      %dx, %al
    and     $0x88, %al
    cmp     $0x08, %al
    jnz     wait_hhd

    mov     %ecx, %eax
    mov     $0x100, %dx
    mul     %dx
    mov     %eax, %ecx
    mov     %edi, %ebx
    mov     $0x1f0, %dx

read_data:
    in      %dx, %ax
    mov     %ax, (%ebx)
    add     $0x2, %ebx
    loop    read_data
    
    ## start to kernel
start_kernel:
    mov     $KERNEL_STACK_ADDR, %esp
    push    $gdt                       # transmit gdt address
    call    KERNEL_LOAD_ADDR
    hlt

    ## gdt defined
gdt:
    GDTSEGASM(0, 0, 0, 0, 0, 0, 0, 0, 0)
    GDTSEGASM(0x0, 0xfffff, 1, 1, 0, 1, 0, 1, 0xa)
    GDTSEGASM(0x0, 0xfffff, 1, 1, 0, 1, 0, 1, 0x2)
gdtdesc:
    .word       (gdtdesc - gdt - 1)
    .long       gdt

    ## boot flags
    .section    ".boot_flags", "a"
flags:
    .byte 0x55, 0xaa