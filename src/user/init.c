#include <libc.h>

int main(void)
{
	creat("init.log", 0);

	while (1)
		;
	return 0;
}
