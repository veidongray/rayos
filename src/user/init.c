#include <ff.h>
#include <libc.h>

int main(void)
{
	int fd = open("/init", FA_CREATE_NEW | FA_READ | FA_WRITE);
	write(fd, "fffffffffffffffffffffffuck", 27);
	sync();

	while (1)
		;
	return 0;
}
