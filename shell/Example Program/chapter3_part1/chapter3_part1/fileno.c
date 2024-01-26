#include <stdio.h>

int main(void)
{
	printf("fd of stdin  = %d\n", fileno(stdin));
	printf("fd of stdout = %d\n", fileno(stdout));
	printf("fd of stderr = %d\n", fileno(stderr));
	return 0;
}
