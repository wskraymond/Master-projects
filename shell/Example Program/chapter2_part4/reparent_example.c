#include <stdio.h>	// printf()
#include <unistd.h>	// getpid(), getppid(), sleep(), fork()
#include <sys/types.h>	// getppid()

int main(void)
{
	int i;
	if(fork() == 0)
	{
		for(i = 0; i < 5; i++)
		{
			printf("(%d) parent's PID = %d\n",
				getpid(),
				getppid()
			);
			sleep(1);
		}
	}
	else
		sleep(1);

	printf("(%d) bye.\n", getpid());
	return 0;
}
