.section .note.GNU-stack,"",@progbits
.section .multiboot2
.align 8
_mboot:
    .long 0xe85250d6                # magic
    .long 0                         # architecture (0 = i386)
    .long _end_mboot - _mboot         # total size
    .long -(0xe85250d6 + 0 + (_end_mboot - _mboot))  # checksum

    # Request memory map information (required!)
    .word 6                         # type = MULTIBOOT_HEADER_TAG_MMAP
    .word 0                         # flags = 0
    .long 8                         # size = 8 bytes

    # End tag (mandatory)
    .word 0                         # type = 0 (end tag)
    .word 0                         # flags = 0
    .long 8                         # size = 8 bytes
_end_mboot:

.code32		# to 32-bit mode

.set KERN_OFFSET, 0xC0100000
.set VIRT_OFFSET, 0xC0000000

.section .text
.global _start
.type _start, @function
_start:
    # save multiboot info address and magic number for later use
    add $(VIRT_OFFSET), %ebx
    mov %ebx, (_mboot_info - VIRT_OFFSET)
    mov %eax, (_mboot_magic - VIRT_OFFSET)

    # initial registers
    xor %eax, %eax
    mov %eax, %ebx
    mov %eax, %ecx
    mov %eax, %edx
    mov %eax, %esi
    mov %eax, %edi
    
    # setup early page
    # map kernel to 0xc0000000+
    mov $(_boot_page_directory - VIRT_OFFSET), %eax
    mov $(_boot_page_table - VIRT_OFFSET), %ebx
    mov %ebx, (%eax)
    orl $0x3, (%eax)
    mov %ebx, 0xc00(%eax)
    orl $0x3, 0xc00(%eax)
    mov %eax, 0xffc(%eax)
    orl $0x3, 0xffc(%eax)

    mov $1024, %ecx
    xor %eax, %eax
    orl $0x3, %eax
    mov $(_boot_page_table - VIRT_OFFSET), %edi
_fill_page_table:
    mov %eax, (%edi)
    add $0x1000, %eax
    add $0x4, %edi
    loop _fill_page_table

    # load page diretory
    mov $(_boot_page_directory - VIRT_OFFSET), %eax
    mov %eax, %cr3

    # enable page
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    ljmp $0x10, $startup_32
startup_32:
    # Set up the stack
    mov $_kstack_top, %esp
    jmp start_kernel
    hlt
    jmp startup_32

.align 4096
.section .data

### early page setup begin ###
.global	_boot_page_directory
_boot_page_directory:
    .space 4096
    
.global	_boot_page_table
_boot_page_table:
    .space 4096
### early page setup end ###

### grub params begin ###
.global _mboot_info
_mboot_info:
    .long 0
.global _mboot_magic
_mboot_magic:
    .long 0
### grub params end ###

### kernel stack begin ###
.global _kstack
_kstack:
    .space 1024 * 1024  # 1MB stack
_kstack_top:
### kernel stack end ###