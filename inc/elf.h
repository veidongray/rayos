#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>

/* ELF Identification */
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_ABIVERSION 8
#define EI_PAD 9
#define EI_NIDENT 16

/* e_ident[EI_MAG] */
#define ELFMAG0 0x7F
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define ELFMAG "\177ELF"
#define SELFMAG 4

/* e_ident[EI_CLASS] */
#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

/* e_ident[EI_DATA] */
#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

/* e_ident[EI_VERSION] */
#define EV_NONE 0
#define EV_CURRENT 1

/* e_ident[EI_OSABI] */
#define ELFOSABI_NONE 0
#define ELFOSABI_LINUX 3
#define ELFOSABI_FREEBSD 9
#define ELFOSABI_STANDALONE 255

/* e_type */
#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4

/* e_machine */
#define EM_NONE 0
#define EM_386 3
#define EM_X86_64 62
#define EM_AARCH64 183
#define EM_RISCV 243

/* e_flags (x86/x86_64 reserved, always 0) */
#define EF_NONE 0

/* Program Header p_type */
#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_TLS 7
#define PT_GNU_EH_FRAME 0x6474e550
#define PT_GNU_STACK 0x6474e551
#define PT_GNU_RELRO 0x6474e552

/* Program Header p_flags */
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

/* Section Header sh_type */
#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_DYNSYM 11

/* Section Header sh_flags */
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MERGE 0x10
#define SHF_STRINGS 0x20
#define SHF_INFO_LINK 0x40
#define SHF_TLS 0x400

/* Special section indices */
#define SHN_UNDEF 0
#define SHN_ABS 0xFFF1
#define SHN_COMMON 0xFFF2

/* Symbol binding (ELF64_ST_BIND / ELF32_ST_BIND) */
#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2

/* Symbol type (ELF64_ST_TYPE / ELF32_ST_TYPE) */
#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_TLS 6

/* Symbol info macros */
#define ELF32_ST_BIND(i) ((i) >> 4)
#define ELF32_ST_TYPE(i) ((i) & 0xF)
#define ELF32_ST_INFO(b, t) (((b) << 4) + ((t) & 0xF))
#define ELF64_ST_BIND(i) ((i) >> 4)
#define ELF64_ST_TYPE(i) ((i) & 0xF)
#define ELF64_ST_INFO(b, t) (((b) << 4) + ((t) & 0xF))

/* Relocation types (x86_64) */
#define R_X86_64_NONE 0
#define R_X86_64_64 1
#define R_X86_64_PC32 2
#define R_X86_64_PLT32 4
#define R_X86_64_GOTPCREL 9
#define R_X86_64_32 10
#define R_X86_64_32S 11

/* Relocation types (i386) */
#define R_386_NONE 0
#define R_386_32 1
#define R_386_PC32 2
#define R_386_GOT32 3
#define R_386_PLT32 4
#define R_386_GOTOFF 9
#define R_386_GOTPC 10

/* Validation helper */
#define IS_ELF(ehdr)                       \
    ((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
     (ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
     (ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
     (ehdr).e_ident[EI_MAG3] == ELFMAG3)

// --------------------
// ELF Program Header Types
// --------------------

#define PT_NULL 0    // 未使用
#define PT_LOAD 1    // 需要加载到内存（最重要）
#define PT_DYNAMIC 2 // 动态链接信息
#define PT_INTERP 3  // 解释器路径（如 /lib/ld.so）
#define PT_NOTE 4    // 附加信息（build-id等）
#define PT_SHLIB 5   // 保留（已废弃）
#define PT_PHDR 6    // Program Header 自身在内存的位置
#define PT_TLS 7     // Thread Local Storage

// GNU 扩展
#define PT_GNU_EH_FRAME 0x6474e550
#define PT_GNU_STACK 0x6474e551
#define PT_GNU_RELRO 0x6474e552
#define PT_GNU_PROPERTY 0x6474e553

// --------------------
// ELF Segment Flags
// --------------------

#define PF_X 0x1 // 可执行
#define PF_W 0x2 // 可写
#define PF_R 0x4 // 可读

struct elf32_ehdr
{
    uint8_t e_ident[EI_NIDENT]; /* ELF 魔数与标识信息 */
    uint16_t e_type;            /* 目标文件类型 (ET_EXEC, ET_DYN等) */
    uint16_t e_machine;         /* 目标架构 (EM_386 = 3) */
    uint32_t e_version;         /* ELF 版本 (EV_CURRENT = 1) */
    uint32_t e_entry;           /* 程序入口点虚拟地址 */
    uint32_t e_phoff;           /* Program Header Table 文件偏移 */
    uint32_t e_shoff;           /* Section Header Table 文件偏移 */
    uint32_t e_flags;           /* 处理器特定标志 (x86通常为0) */
    uint16_t e_ehsize;          /* ELF Header 大小 (52字节) */
    uint16_t e_phentsize;       /* 单个 Program Header 大小 (32字节) */
    uint16_t e_phnum;           /* Program Header 数量 */
    uint16_t e_shentsize;       /* 单个 Section Header 大小 (40字节) */
    uint16_t e_shnum;           /* Section Header 数量 */
    uint16_t e_shstrndx;        /* Section Name String Table 索引 */
};

struct elf64_ehdr
{
    uint8_t e_ident[EI_NIDENT]; /* ELF 魔数与标识信息 */
    uint16_t e_type;            /* 目标文件类型 */
    uint16_t e_machine;         /* 目标架构 (EM_X86_64 = 62) */
    uint32_t e_version;         /* ELF 版本 */
    uint64_t e_entry;           /* 程序入口点虚拟地址 */
    uint64_t e_phoff;           /* Program Header Table 文件偏移 */
    uint64_t e_shoff;           /* Section Header Table 文件偏移 */
    uint32_t e_flags;           /* 处理器特定标志 */
    uint16_t e_ehsize;          /* ELF Header 大小 (64字节) */
    uint16_t e_phentsize;       /* 单个 Program Header 大小 (56字节) */
    uint16_t e_phnum;           /* Program Header 数量 */
    uint16_t e_shentsize;       /* 单个 Section Header 大小 (64字节) */
    uint16_t e_shnum;           /* Section Header 数量 */
    uint16_t e_shstrndx;        /* Section Name String Table 索引 */
};

struct elf64_phdr
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

#endif // ELF_H