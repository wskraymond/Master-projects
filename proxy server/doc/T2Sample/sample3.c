/*
 * This server as an example on how to passing arguments between threads
 */
# include <stdio.h>
# include <stdlib.h>
# include <pthread.h>
# include <unistd.h>

# define THREADNUM 3

typedef struct threadargs{
	int a;
	int b;
}threadargs;

void* pthread_prog(void * args){
	threadargs* tas=(threadargs*)args;
	printf("This is a thread and the two parameters: %d %d\n",tas->a,tas->b);
	pthread_exit(NULL);
	return 0;
}

int main(int argc, char** argv){
	//threadargs is the arguments structure
    int i;
	threadargs tas[THREADNUM];
    for(i=0;i<THREADNUM;i++){
	    tas[i].a=i+1;
	    tas[i].b=(i+1)*(i+1);
    }
	
	pthread_t thread[THREADNUM];
    for(i=0;i<THREADNUM;i++){
	    int return_value=pthread_create(&thread[i],NULL,pthread_prog,&tas[i]);
    }
	
	/*while(1){
		printf("this is master thread\n");
	}*/
	
    for(i=0;i<THREADNUM;i++){
	    pthread_join(thread[i],NULL);
    }
	return 0;
}
