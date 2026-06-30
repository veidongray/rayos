#include <init.h>
#include <printk.h>

/* 由链接器脚本提供的外部符号 */
extern initcall_t __initcall_start[];
extern initcall_t __initcall_end[];
extern exitcall_t __exitcall_start[];
extern exitcall_t __exitcall_end[];

extern initcall_t __initcall_0_start[];
extern initcall_t __initcall_1_start[];
extern initcall_t __initcall_2_start[];
extern initcall_t __initcall_3_start[];
extern initcall_t __initcall_4_start[];
extern initcall_t __initcall_5_start[];
extern initcall_t __initcall_6_start[];
extern initcall_t __initcall_7_start[];

extern exitcall_t __exitcall_7_start[];
extern exitcall_t __exitcall_6_start[];
extern exitcall_t __exitcall_5_start[];
extern exitcall_t __exitcall_4_start[];
extern exitcall_t __exitcall_3_start[];
extern exitcall_t __exitcall_2_start[];
extern exitcall_t __exitcall_1_start[];
extern exitcall_t __exitcall_0_start[];

static const char *calls_level_names[] = {
    "pure",
    "core",
    "postcore",
    "arch",
    "subsys",
    "fs",
    "device",
    "late",
};

static inline int get_initcall_level(initcall_t fn)
{
    if (fn >= (initcall_t)__initcall_7_start)
        return 7;
    if (fn >= (initcall_t)__initcall_6_start)
        return 6;
    if (fn >= (initcall_t)__initcall_5_start)
        return 5;
    if (fn >= (initcall_t)__initcall_4_start)
        return 4;
    if (fn >= (initcall_t)__initcall_3_start)
        return 3;
    if (fn >= (initcall_t)__initcall_2_start)
        return 2;
    if (fn >= (initcall_t)__initcall_1_start)
        return 1;
    return 0;
}

static inline int get_exitcall_level(exitcall_t fn)
{
    if (fn >= (exitcall_t)__exitcall_7_start)
        return 7;
    if (fn >= (exitcall_t)__exitcall_6_start)
        return 6;
    if (fn >= (exitcall_t)__exitcall_5_start)
        return 5;
    if (fn >= (exitcall_t)__exitcall_4_start)
        return 4;
    if (fn >= (exitcall_t)__exitcall_3_start)
        return 3;
    if (fn >= (exitcall_t)__exitcall_2_start)
        return 2;
    if (fn >= (exitcall_t)__exitcall_1_start)
        return 1;
    return 0;
}

void do_initcalls(void)
{
    initcall_t *fn;
    size_t count;

    for (fn = __initcall_start, count = 0; fn < __initcall_end; fn++, count++)
    {
        int ret = (*fn)();
        printk("do_initcalls: %lu initcall in %s level",
               count, calls_level_names[get_initcall_level((initcall_t)fn)]);

        if (ret != 0)
        {
            printk("do_initcalls: initcall %p returned error %d", (void *)*fn, ret);
        }
    }
}

void do_exitcalls(void)
{
    exitcall_t *fn;
    size_t count;

    /* 正向遍历即可，因为链接脚本已经倒序排列 */
    for (fn = __exitcall_start, count = 0; fn < __exitcall_end; fn++, count++)
    {
        printk("do_exitcalls: %lu exitcall in %s level",
               count, calls_level_names[get_exitcall_level((exitcall_t)fn)]);
        (*fn)();
    }
}