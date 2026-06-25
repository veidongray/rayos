#ifndef INIT_H
#define INIT_H

typedef int (*initcall_t)(void);

/*
 * 将函数指针放入指定的 .initcall.N 段
 * __used__        : 防止编译器优化掉这个"未被引用"的变量
 * __section__()   : 指定 ELF section
 * __aligned__()   : 保证指针对齐（x86_64 下为 8 字节）
 */
#define __define_initcall(fn, level)            \
    static initcall_t __initcall_##fn##_##level \
        __attribute__((used, section(".initcall." #level), aligned(sizeof(void *)))) = fn

/* 分级宏，数字越小越先执行 */
#define pure_initcall(fn) __define_initcall(fn, 0)
#define core_initcall(fn) __define_initcall(fn, 1)
#define postcore_initcall(fn) __define_initcall(fn, 2)
#define arch_initcall(fn) __define_initcall(fn, 3)
#define subsys_initcall(fn) __define_initcall(fn, 4)
#define fs_initcall(fn) __define_initcall(fn, 5)
#define device_initcall(fn) __define_initcall(fn, 6)
#define late_initcall(fn) __define_initcall(fn, 7)

/* 默认级别，等价于 Linux 的 module_init() */
#define module_init(fn) device_initcall(fn)

typedef void (*exitcall_t)(void);

#define __define_exitcall(fn, level)            \
    static exitcall_t __exitcall_##fn##_##level \
        __attribute__((used, section(".exitcall." #level), aligned(sizeof(void *)))) = fn

#define pure_exitcall(fn) __define_exitcall(fn, 0)
#define core_exitcall(fn) __define_exitcall(fn, 1)
#define postcore_exitcall(fn) __define_exitcall(fn, 2)
#define arch_exitcall(fn) __define_exitcall(fn, 3)
#define subsys_exitcall(fn) __define_exitcall(fn, 4)
#define fs_exitcall(fn) __define_exitcall(fn, 5)
#define device_exitcall(fn) __define_exitcall(fn, 6)
#define late_exitcall(fn) __define_exitcall(fn, 7)

/* 默认级别 */
#define module_exit(fn) __define_exitcall(fn, 6)

void do_initcalls(void);
void do_exitcalls(void);

#endif /* INIT_H */