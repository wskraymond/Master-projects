#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	char *ptr1, *ptr2;
	ptr1 = malloc(16);
	ptr2 = malloc(16);

	printf("Distance between ptr1 and ptr2: %d bytes\n",
		ptr2 - ptr1);
	return 0;
}
