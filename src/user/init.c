#include <syscall.h>

int main(void)
{
    write(STDOUT, "Hello! I'm User!", 17);

    while (1)
        ;
    return 0;
}
