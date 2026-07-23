#include <libc.h>

int main(void)
{
	// 运行一段时间然后退出
	for (int i = 0; i < 0xfffffff; i++)
		;
	exit(0);
}
