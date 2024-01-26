#include <stdio.h>	// required by printf()
#include <string.h>	// required by memset()

#define ONE_MEG	(1024 * 1024)

char a[2048 * ONE_MEG];

int main(void) {
	memset(a, 0, sizeof(a));
	printf("2GB OK\n");
	return 0;
}
