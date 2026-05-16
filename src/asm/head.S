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
    # 因为 GRUB 将 info 和 magic 的地址分别放到 ebx 和 eax 中
    # 这里先将两个寄存器的内容存到_mboot_info和_mboot_magic中
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
    # 因为连接时已经连接到0xc0000000以上的地址
    # 为了防止后面的符号访问出错
    # 所以要先初始化一部分分页
    # 保证在系统初始化的时候能够正常访问0xc0000000以上的地址
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

    # 这一步的跳转主要是为了刷新 CPU 指令缓存
    ljmp $0x10, $startup_32
startup_32:
    # Set up the stack
    mov $_kstack_top, %esp
    jmp start_kernel
    hlt
    jmp startup_32

.align 4096
.section .data

# 这两个地址空间用来保存boot时的页配置
# 因为 boot 后需要马上映射一部分地址到0xc0000000
.global	_boot_page_directory
_boot_page_directory:
    .space 4096
.global	_boot_page_table
_boot_page_table:
    .space 4096

# 用来存储 multiboot 信息的空间
.global _mboot_info
_mboot_info:
    .long 0
.global _mboot_magic
_mboot_magic:
    .long 0

# 这个栈空间只会在第一个任务启动前使用
# 也就是系统初始化的时候会使用
# 第一个任务启动之后后面的任务都会使用自己的栈空间
# 所以不需要太大
.global _kstack
_kstack:
    .space 128 * 1024  # 128KB stack
_kstack_top:
### kernel stack end ###