#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);

// ------------------------
// 16位宽字符操作
// ------------------------

size_t wcslen(const uint16_t *s);
uint16_t *wcscpy(uint16_t *dest, const uint16_t *src);
uint16_t *wcsncpy(uint16_t *dest, const uint16_t *src, size_t n);
int wcscmp(const uint16_t *s1, const uint16_t *s2);
int wcsncmp(const uint16_t *s1, const uint16_t *s2, size_t n);
uint16_t *wcschr(const uint16_t *s, uint16_t c);
uint16_t *wcsrchr(const uint16_t *s, uint16_t c);

size_t wcslcpy(uint16_t *dst, const uint16_t *src, size_t size);
uint16_t *wcscat(uint16_t *dest, const uint16_t *src);

#endif /* STRING_H */