#include "gdt_asm.h"
#include "page_table.h"
    .section .text
    .global entry
entry:
    push %ebp
    mov %esp, %ebp
    push 0x8(%ebp)
    call copy_gdt
    lgdt new_gdtdesc
    call create_page
    mov %cr4, %eax
    or $0x10, %eax
    mov %eax, %cr4
    mov $PDT_BASE, %eax
    mov %eax, %cr3
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0
    call main
    leave
    ret
    
    .global copy_gdt
copy_gdt:
    push %ebp
    mov %esp, %ebp
    ## copy gdt to 0x20000, size 512Bytes
    push %esi
    mov 0x8(%ebp), %esi
    push %edi
    mov $GDT_COPYADDR, %edi
    push %ecx
    mov $0x80, %ecx
s0:
    mov (%esi), %eax
    mov %eax, (%edi)
    add $0x4, %esi
    add $0x4, %edi
    loop s0
    pop %ecx
    pop %edi
    pop %esi
    leave
    ret

    .global create_page
create_page:
    push %ebp
    mov %esp, %ebp
    ## create PDE and PTE
    ## PDE: 0x30000 ~ 0x30fff, 4KB
    ## PTE: 0x31000 ~ 0x3ffff, 60KB
    ## PM: 0x7 => U/S[1], R/W[1], P[1]
    push %edi
    push %esi
    mov $PDT_BASE, %edi
    mov $0x31000, %esi
    or $0x7, %esi
s1:
    mov %esi, (%edi)
    add $0x4, %edi
    add $0x1000, %esi
    cmp $0x31000, %edi
    jne s1
    mov $PT_BASE, %edi
    mov $0x0, %esi
    or $0x7, %esi
s2:
    mov %esi, (%edi)
    add $0x4, %edi
    add $0x1000, %esi
    cmp $0x40000, %edi
    jne s2
    pop %esi
    pop %edi
    leave
    ret

    .global putc
putc:
    push %ebp
    mov %esp, %ebp
    push %edi
    push %esi
    push %eax
    mov $0xb8000, %edi
    mov 0x8(%ebp), %al
    mov %al, (%edi)
    pop %eax
    pop %esi
    pop %edi
    leave
    ret
    .section .data
new_gdtdesc:
    .word GDT_SIZE
    .long GDT_COPYADDR