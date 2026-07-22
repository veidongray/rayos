#include <ff.h>
#include <libc.h>

int main(void)
{
	int fd = open("/stdout", FA_READ | FA_WRITE);
	if (fd >= 0) {
		write(fd, "fffffffffffffffffffffffuck", 27);
		sync();
	}

	exit(1234);
	while (1)
		;
	return 0;
}
