/*
 * This is gonna show how global variables and private variables works in pthread programming
 */
# include <stdio.h>
# include <stdlib.h>
# include <pthread.h>
# include <unistd.h>

# define THREADNUM 10
# define USING_MUTEX 0

int global=100;
pthread_mutex_t mutex;

void* pthread_prog(void * args){
	int* thread_num_p=(int*) args;
	int thread_num=*thread_num_p;
	int private=2;
	if(USING_MUTEX==1){
		pthread_mutex_lock(&mutex);
	}
	//sleep(thread_num);
	printf("From worker thread %d BEFORE: global is %d and private is %d\n",thread_num,global++,private++);
	printf("From worker thread %d AFTER: global is %d and private is %d\n",thread_num,global,private);
	if(USING_MUTEX==1){
		pthread_mutex_unlock(&mutex);
	}
	pthread_exit(NULL);
	return 0;
}

int main(int argc, char** argv){
	pthread_t thread[THREADNUM];
    pthread_mutex_init(&mutex,NULL);
	int private=1;
	int i;
	int arg[THREADNUM];
    for(i=0;i<THREADNUM;i++){
        arg[i]=i+1;
    }
	for(i=0;i<THREADNUM;i++){
		int ret_val=pthread_create(&thread[i],NULL,pthread_prog,&arg[i]);
	}
	if(USING_MUTEX==1){
		pthread_mutex_lock(&mutex);
	}
	printf("From master thread: global is %d and private is %d\n",global,private);
	global++;
    private++;
	printf("From master thread: global is %d and private is %d\n",global,private);
	if(USING_MUTEX==1){
		pthread_mutex_unlock(&mutex);
	}
	for(i=0;i<THREADNUM;i++)
	pthread_join(thread[i],NULL);
    printf("MASTER THREAD: Global is %d\n",global);
	return 0;
}
