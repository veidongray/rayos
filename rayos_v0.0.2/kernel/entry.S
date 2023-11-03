#include "entry.h"

    .code32
    .align 4
    .section .text
    .global entry
entry:
    push $0x0
    call set_cursor
    print(welcome_info)
    ## set page direct
    mov $0x2000, %edi   # page direct address
    mov $0x3000, %eax    # first page direct element => 0x3000 phyaddr
    or $0x7, %eax       # set U/S, R/W, P bits to 1
    mov %eax, (%edi)
    add $0xfff, %edi   # end page direct element => 0x3000 phyaddr
    mov %eax, (%edi)
    ## set page table
    mov $0x3000, %edi   # page 1 table address
    mov $0x0000, %eax   # first element value => 0x0000_0000 phyaddr
    mov $0x400, %ecx    # set a number of 1024 page table element
    or $0x7, %eax       # set U/S, R/W, P bits to 1
ptset:
    mov %eax, (%edi)
    add $0x1000, %eax
    add $0x4, %edi
    loop ptset
    ## open Page support
    mov $0x2000, %eax
    mov %eax, %cr3
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0
    mov $10, %eax
    call main
    ret

    .section .data
done_info:
    .string " [OK] \n"
welcome_info:
    .string "                         --- Welcome to OS! ---                                 \n"
boot_info:
    .string "Kernel bootting ==> \n"
init_gdt_info:
    .string "Initial GDT...\n"
init_page_info:
    .string "Initial Page...\n"