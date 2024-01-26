#include "int_header.h"
#include <stdio.h>	// required by printf()

int main(void)
{
	printf("3 + 10 = %d\n", add_int(3, 10));
	printf("3 * 10 = %d\n", multi_int(3, 10));
	return 0;
}
