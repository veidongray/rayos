#include <lib/string/string.h>

// ------------------------
// 内存操作
// ------------------------

void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--)
        *d++ = *s++;
    return dest;
}

void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--)
        *p++ = (unsigned char)c;
    return s;
}

void *memmove(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (d < s)
    {
        // 从前往后拷贝（无重叠或 src 在 dest 前）
        while (n--)
            *d++ = *s++;
    }
    else
    {
        // 从后往前拷贝（防止重叠覆盖）
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *a = (const unsigned char *)s1;
    const unsigned char *b = (const unsigned char *)s2;
    while (n--)
    {
        if (*a != *b)
            return *a - *b;
        a++;
        b++;
    }
    return 0;
}

// ------------------------
// 字符串操作
// ------------------------

size_t strlen(const char *s)
{
    const char *p = s;
    while (*p)
        p++;
    return p - s;
}

char *strcpy(char *dest, const char *src)
{
    char *ret = dest;
    while ((*dest++ = *src++) != '\0')
        ;
    return ret;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    char *ret = dest;
    while (n && (*dest++ = *src++))
    {
        n--;
    }
    while (n--)
    {
        *dest++ = '\0';
    }
    return ret;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    while (n && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
        n--;
    }
    if (n == 0)
        return 0;
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

char *strchr(const char *s, int c)
{
    while (*s)
    {
        if (*s == (char)c)
            return (char *)s;
        s++;
    }
    return (char *)(c == '\0' ? s : NULL);
}

char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    do
    {
        if (*s == (char)c)
            last = s;
    } while (*s++);
    return (char *)last;
}