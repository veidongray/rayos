#ifndef BASE_TYPES_H
#define BASE_TYPES_H

/* ==================== 精确宽度整数类型 ==================== */
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

/* ==================== 整数范围宏 ==================== */
/* 8-bit */
#define INT8_MIN (-128)
#define INT8_MAX 127
#define UINT8_MAX 255

/* 16-bit */
#define INT16_MIN (-32768)
#define INT16_MAX 32767
#define UINT16_MAX 65535

/* 32-bit */
#define INT32_MIN (-2147483647 - 1) // 避免编译器警告
#define INT32_MAX 2147483647
#define UINT32_MAX 4294967295U

/* 64-bit */
#define INT64_MIN (-9223372036854775807LL - 1LL)
#define INT64_MAX 9223372036854775807LL
#define UINT64_MAX 18446744073709551615ULL

/* ==================== 最小/最快类型 (可选，按需启用) ==================== */
/*
typedef int8_t   int_least8_t;
typedef int16_t  int_least16_t;
typedef int32_t  int_least32_t;
typedef int64_t  int_least64_t;

typedef uint8_t  uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;

typedef int32_t  int_fast8_t;
typedef int32_t  int_fast16_t;
typedef int32_t  int_fast32_t;
typedef int64_t  int_fast64_t;

typedef uint32_t uint_fast8_t;
typedef uint32_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
typedef uint64_t uint_fast64_t;
*/

/* ==================== 指针相关类型 ==================== */
#ifndef NULL
#define NULL ((void *)0)
#endif

// 根据平台自动选择 size_t / ptrdiff_t
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(__LP64__)
typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef unsigned long int uintptr_t;
#else
typedef int ptrdiff_t;
typedef unsigned int size_t;
#endif

#define offsetof(type, member) \
    ((size_t)((char *)&((type *)0)->member - (char *)0))

/* ==================== 最大整数类型 ==================== */
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;

#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX

#endif // BASE_TYPES_H