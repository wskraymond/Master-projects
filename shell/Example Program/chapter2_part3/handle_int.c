#include <stdio.h>	// printf(), getchar()
#include <signal.h>	// signal(), "SIGINT"

void sig_handler(int sig)
{
	if(sig == SIGSEGV)
		printf("I've received SEG. FAULT\n");
	if(sig == SIGINT)
		printf("I've received Ctrl + C\n");
}

int main(void)
{
	signal(SIGINT, sig_handler);
	signal(SIGSEGV, sig_handler);
	printf("Press enter\n");
	getchar();
	printf("End of program\n");
	return 0;
}
