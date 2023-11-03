#include "asm.h"
#define GDTCSLE (0x1 << 3)
#define GDTDSLE (0x2 << 3)
#define SECTS 0x800000              # 4MB
#define ENTRYDEST 0x100000
#define STACKDEST 0x200000

    .code16
    .section ".text", "ax"
    .global start
start:
    cli
    xor     %eax, %eax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %ss
    mov     %ax, %fs
    mov     %ax, %gs

seta20.1:
    inb     $0x64,%al               # Wait for not busy
    testb   $0x2,%al
    jnz     seta20.1

    movb    $0xd1,%al               # 0xd1 -> port 0x64
    outb    %al,$0x64

seta20.2:
    inb     $0x64,%al               # Wait for not busy
    testb   $0x2,%al
    jnz     seta20.2

    movb    $0xdf,%al               # 0xdf -> port 0x60
    outb    %al,$0x60

    lgdt    gdtdesc

    movl    %cr0, %eax
    orl     $0x1, %eax
    movl    %eax, %cr0

    ljmp    $GDTCSLE, $start32
    .code32
start32:
    mov     $GDTDSLE, %eax
    mov     %eax, %ds
    mov     %eax, %es
    mov     %eax, %ss
    xor     %eax, %eax
    mov     %eax, %fs
    mov     %eax, %gs

read_kernel:
    mov     $0x1, %ebx              # sector logic number
    mov     $0x10, %ecx             # read sectors
    mov     $ENTRYDEST, %edi        # dest address => 1MB

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

start_kernel:
    mov     $STACKDEST, %esp
    call    ENTRYDEST
    hlt

# Bootstrap GDT
    .section ".gdt", "a"
    .p2align 2                              # force 4 byte alignment
gdt:
    SEG_NULLASM                             # null seg
    SEG_ASM(STA_X | STA_R, 0x0, 0xffffffff) # code seg
    SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg

gdtdesc:
    .word   (gdtdesc - gdt - 1)             # gdt limit by gdt size
    .long   gdt                             # address gdt

    .section ".bootflag", "a"
    .global _bf
_bf:   .word 0xaa55