#include <unistd.h>	// alarm()

int main(int argc, char **argv)
{
	alarm(1);
	while(1) puts("hey");
	return 0;
}
