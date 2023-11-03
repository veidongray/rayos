#include "types.h"

static uchar *str = "Loading kernel...        ";

void main(void)
{
    puts(str);
    while (1);
}