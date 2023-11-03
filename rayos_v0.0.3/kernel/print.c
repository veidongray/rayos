#include "print.h"
#include "cursor.h"

void puts(const char *str)
{
    int i;
    int len;

    for (len = 0; str[len] != '\0'; len++);
    for (i = 0; i < len; i++) {
        putc(str[i]);
    }
}