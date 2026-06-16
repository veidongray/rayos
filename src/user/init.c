#include <syscall.h>

int main(void)
{
    write(1, "Hello! I'm User!\n", 18);

    while (1)
        ;
    return 0;
}
