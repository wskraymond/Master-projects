
#include <stdio.h>	// perror()
#include <unistd.h>	// pause()
#include <signal.h>	// signal(), "SIGINT"

void sig_handler(int sig)
{
	if(sig == SIGINT)
		printf("SIGINT received\n");
}

int main(void)
{
	char c = 'a';
	signal(SIGINT, sig_handler);
	pause();
	perror("pause()");
	printf("Terminated normally.\n");
	wait(NULL);
	perror("wait()");
	write(fileno(stdout), &c, 1);
	perror("write()");
	return 0;
}
