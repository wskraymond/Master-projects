#include <stdio.h>

int main(int argc, char **argv)
{
	char c;
	while( (c = getchar()) != EOF )
	{
		fflush(stdin);
		putchar(c);
	}
	return 0;
}
