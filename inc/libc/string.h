#ifndef STRING_H
#define STRING_H

int strlen(char *str);
void *memset(void *str, int c, int n);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
void *memcpy(void *dest, const void *src, int n);

#endif // STRING_H