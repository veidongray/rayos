#ifndef PRINTK_H
#define PRINTK_H

#include <printf.h>

#define PR_DEBUG 1

#if PR_DEBUG
#define printk(fmt, ...)                                                      \
    do                                                                        \
    {                                                                         \
        printf("[%s:%d]: " fmt "\n", __FILE__, (int)__LINE__, ##__VA_ARGS__); \
    } while (0)
#else
#define printk(fmt, ...)                 \
    do                                   \
    {                                    \
        printf(fmt "\n", ##__VA_ARGS__); \
    } while (0)
#endif

/* pr_xxx */
#define pr_err(fmt, ...) printk("[ERR] " fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...) printk("[WARN] " fmt, ##__VA_ARGS__)
#define pr_info(fmt, ...) printk("[INFO] " fmt, ##__VA_ARGS__)

#if PR_DEBUG
#define pr_debug(fmt, ...) printk("[DBG] " fmt, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...) ((void)0)
#endif

#endif // PRINTK_H