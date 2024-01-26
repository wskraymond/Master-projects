#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
	char buf[10];
	int ret;

	while(1)
	{
		ret = read(fileno(stdin), buf, 10);
		printf("ret = %d bytes.\n", ret);
		if(ret <= 0)
			break;

		printf("output = |");
		fflush(stdout);
		write(fileno(stdout), buf, 10);	// should it be 10?
		printf("|\n");
	}

	printf("bye!\n");
	return 0;
}
