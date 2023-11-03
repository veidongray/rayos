#include "x86.h"
#include "cursor.h"
#include "print.h"

static char ch = 'K';
static int i = 0, j = 0;

int main(void)
{
    for (i = 0; i < 20; i++) {
        for (j = 0; j < 80; j++) {
            set_cursor(j, i);
            putc(ch);
        }
    }
    while (1);
    return 0;
}