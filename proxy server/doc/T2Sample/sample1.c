/*
 * This is sample code displaying the multi-thread programming
 * for CSCI4430
 */
# include <stdio.h>
# include <stdlib.h>
# include <pthread.h>
# include <unistd.h>

void* pthread_prog(void * args){
    int threadId=*(int*) args;
    sleep(threadId);
    printf("This is worker thread %d\n",threadId);
	return 0;
}

int main(int argc, char** argv){
	pthread_t thread[3];
	int private=1;
	int i;
	int arg[3];
	arg[0]=1;
	arg[1]=2;
	arg[2]=3;
	for(i=0;i<3;i++){
		int ret_val=pthread_create(&thread[i],NULL,pthread_prog,&arg[i]);
	}
	printf("This is master thread\n");
	for(i=0;i<3;i++)
	pthread_join(thread[i],NULL);
	return 0;
}
