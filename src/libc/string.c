#include "libc/string.h"

int strlen(char *str)
{
    char *ptr = (char *)str;
    int len = 0;
    while (*ptr != '\0')
    {
        len++;
        ptr++;
    }
    return len;
}

void *memset(void *str, int c, int n)
{
    unsigned char *ptr = str;
    int len = 0;

    while (len < n)
    {
        *ptr = c;
        ptr++;
        len++;
    }
    return str;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 == *s2)
    {
        if (*s1 == '\0')
            return 0;
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

char *strcpy(char *dest, const char *src)
{
    char *ret = dest;
    while ((*dest++ = *src++) != '\0')
        ;
    return ret;
}

void *memcpy(void *dest, const void *src, int n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    void *ret = dest;

    while (n--)
    {
        *d++ = *s++;
    }
    return ret;
}