#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void * hello(void *input)
{
	int i;
	for(i = 0; i < 100000000; i++)
	{
		if(i == 10000000)
			printf("DAM!\n");
		if(i == 50000000)
			printf("BAM!\n");
		if(i == 100000000-1)
			printf("GAM!\n");
	}

	printf("%s\n", (char *) input);
	pthread_exit(NULL);
}

int main(void)
{
	int i;
	pthread_t tid;
	pthread_create(&tid, NULL, hello, "hello world");

	for(i = 0; i < 100000000; i++)
	{
		if(i == 10000000)
			printf("DAM\n");
		if(i == 50000000)
			printf("BAM\n");
		if(i == 100000000-1)
			printf("GAM\n");
	}

	pthread_join(tid, NULL);
	printf("main thread terminated\n");
	return 0;
}
