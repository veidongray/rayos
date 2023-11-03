#ifndef __BOOT_H__
#define __BOOT_H__

#include "gdt_asm.h"

#define LOAD_SEC_LOGIC      0x1
#define LOAD_SEC_NUM        0x40
#define KERNEL_LOAD_ADDR    0x100000
#define KERNEL_STACK_ADDR   0x10000

#define KERNEL_GDT_CODESEG  (0x1 << 3)
#define KERNEL_GDT_DATASEG  (0x2 << 3)

#endif //__BOOT_H__