# include <stdio.h>
# include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "myftp.h"
#include <dirent.h>
#include <limits.h>
# include <pthread.h>

#define MAX 10 //test!!!!
#define THROUGHPUT 1000
extern char** readDir(char*);

typedef struct threadargs{
	pthread_t client_ID;
	int client_index;
	int client_sd;
	struct sockaddr_in client_addr;
}threadargs;

//-----Threads' private-------
threadargs args[MAX];
//----Shared Objects-----
int available[MAX];
int ind;
int m;
//-----------------------
//-----Semaphores--------
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // for index and available
pthread_cond_t 	full	= PTHREAD_COND_INITIALIZER;	// vacancy is full
//-----------------------

int msgReceiver(int client_sd,struct message_s* msg_in)
{
	int len;
	memset(msg_in,0,sizeof(struct message_s));
	if((len=recv(client_sd,msg_in,sizeof(struct message_s),0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	//network to host byte
	msg_in->length = ntohl(msg_in->length);
	//printf("bytes received: %d\n",len);
	return len;
}

int msgSender(int client_sd,struct message_s* msg_out)
{
	int len;
	if((len=send(client_sd,msg_out,sizeof(struct message_s),0))<0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	//printf("bytes sent: %d\n",len);
	return len;
}

int dataReceiver(int client_sd,int size,char* buf)
{
	int len=0;
	int recvlength=0;
	memset(buf,0,sizeof(buf));
	while(recvlength<size){
		if((len=recv(client_sd,buf+recvlength,size-recvlength,0))<0){
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
		}
		//printf("recv...\n");
		recvlength+=len;
	}
	//printf("bytes received: %d\n",len);
	return recvlength;
}

int dataSender(int client_sd,int size,char* buf)
{
	int len;
	int sentlength=0;
	while(sentlength<size){
		if((len=send(client_sd,buf+sentlength,size-sentlength,0))<0){
			printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		sentlength += len;
	}
	//printf("bytes sent: %d\n",len);
	
	return sentlength;
}

void connection(threadargs* client_info,struct message_s* msg_in,struct message_s* msg_out)
{
	int len;
	int error;
	//printf("connection in progress\n");
	
	len = msgReceiver(client_info->client_sd,msg_in);
	if(error=msgChecker(msg_in,OPEN_CONN_REQUEST,unused,len))
	{
		errorprint(error);
		exit(0);
	}	
	
	*msg_out = msgGeneration(OPEN_CONN_REPLY,1,0);
	msgSender(client_info->client_sd,msg_out);
	//printf("connection established\n");
	//show client sock addr.
	char* str_client_addr = inet_ntoa(client_info->client_addr.sin_addr);
	unsigned short client_port = client_info->client_addr.sin_port;
	printf("Connected client[%d] info=> IP:%s  Port number:%hu \n", client_info->client_index,str_client_addr, client_port);
}

int authentication(int client_sd,FILE* access_file,struct message_s* msg_in,struct message_s* msg_out,char* buf)
{
	int status;
	int error;
	int len;
	//printf("authentication in progress\n");
	
	len = msgReceiver(client_sd,msg_in);
	if(error=msgChecker(msg_in,AUTH_REQUEST,unused,len))
	{
		errorprint(error);
		exit(0);
	}	
	
	//printf("%d\n",msg_in->length-12);
	len = dataReceiver(client_sd,msg_in->length-12,buf);

	if(len != (msg_in->length-12) )
	{
		printf("recieve not enough bytes len:%d length:%d\n",len,msg_in->length-12);
		exit(0); //error
	}
	//username password checking	
	
	char user_pwd[80];
	status = 0;
	
	char* nlptr;
	while(fgets(user_pwd,80,access_file)!=NULL)
	{

		nlptr = strrchr ( user_pwd, '\n' ) ;
		if( nlptr != 0 )
		{
    			*nlptr = '\0' ;
		}

		if(strcmp(user_pwd,buf)==0)
		{
			status = 1;
			break;
		}
	}// end of file
	
	*msg_out = msgGeneration(AUTH_REPLY,status,0);
	msgSender(client_sd,msg_out);
	
	if(status)
	{
		printf("authentication accepted\n");
		return 0; //success
	}
	else
		printf("authentication rejected\n");
	return 1; //failure
}

int listGen(char* buf)
{
    char** c = readDir("filedir/");
    int index=0;
	
	memset(buf,0,sizeof(buf));
	buf[0] = '\0';
    while(c[index]!= NULL){
		if(strcmp(c[index],".")!=0 && strcmp(c[index],"..")!=0)
		{
			strcat(c[index],"\n");
			strcat(buf,c[index]);
		}
		index++;
	}
	return strlen(buf);
}

void listFile(int client_sd,struct message_s* msg_out,char* buf)
{
	int payload;
	//printf("List service\n");
	payload = listGen(buf)+ 1;
	if(payload==1)
		printf("------empty directory-------\n");
	*msg_out = msgGeneration(LIST_REPLY,unused,payload);
	msgSender(client_sd,msg_out);
	dataSender(client_sd,payload,buf);
	//printf("List done\n");
}

int fileSearch(char* filename)
{
	char** c = readDir("filedir/");
    int index=0;
	
    while(c[index]!= NULL){
			if(strcmp(filename,c[index])==0)
				return 1;
        index++;
    }
	//free malloc!!!!!!!!!!
	
	return 0;
}

int data_out(int d_fd,int size,char* buf)
{
	int len;
	memset(buf,0,sizeof(buf));
	if((len=read(d_fd,buf,size))<0){
		printf("read error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	//printf("data_out:\n %s\n", buf);
	return len;
}

void beDownload(int client_sd,struct message_s* msg_in,struct message_s* msg_out,char* buf)
{
	int len;
	int status = 1;
	int payload;
	int d_fd;
	char filename[256+1];
	char path[256 + 1 + 10] = "filedir/"; 
	//printf("Download service\n");
	
	len = dataReceiver(client_sd,msg_in->length-12,buf);
	if(len != (msg_in->length-12) )
		exit(0); //error
	strcpy(filename,buf);
	if(fileSearch(filename) == 1)
	{
		strcat(path,filename);
		if((d_fd = open(path,O_RDONLY,S_IRUSR)) == -1){
			printf("open d_file erro: %s (Errno:%d)\n",strerror(errno),errno);
			status = 0;
			payload = 0;
		}
	}
	else{
		status = 0;
		payload = 0;		
	}
	
	//reply
	*msg_out = msgGeneration(GET_REPLY,status,0);
	msgSender(client_sd,msg_out);
	
	//file exits , send msg and payload.
	if(status == 1)
	{
		//payload setting // check file size
		struct stat s_file;
		if(fstat(d_fd,&s_file)==-1)
		{
			printf("file status erro: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		payload = s_file.st_size;
		*msg_out = msgGeneration(FILE_DATA,unused,payload);
		msgSender(client_sd,msg_out);
		
		//buffering
		int count = payload;
		//block of data deliver
		for(count=count-THROUGHPUT;count>0;count=count-THROUGHPUT){
			data_out(d_fd,THROUGHPUT,buf);
			dataSender(client_sd,THROUGHPUT,buf);
		}
		count = count + THROUGHPUT;
		//remain > 0 but <= THROUGHPUT
		data_out(d_fd,count,buf);
		dataSender(client_sd,count,buf);
	}
	//printf("Doenload done\n");
}

int data_in(int u_fd,int size,char* buf)
{
	int len;
    if((len=write(u_fd,buf,size))==-1){
		printf("write error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	//printf("data_in: %s\n", buf);
	return len;
}
void beUpload(int client_sd,struct message_s* msg_in, struct message_s* msg_out,char* buf)
{
	int payload;
	int len;
	int u_fd;
	char filename[256+1];
	char path[256+1 + 10] = "filedir/";
	//printf("Upload service\n");
	
	len = dataReceiver(client_sd,msg_in->length-12,buf);
	if(len != (msg_in->length-12) )
		exit(0); //error
	
	//set path
	strcpy(filename,buf);
	strcat(path,filename);
	
	//create uploaded file
	if( (u_fd = open(path,O_WRONLY|O_CREAT,S_IRWXU)) == -1)
	{
		printf("open u_file erro: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	
	//reply
	*msg_out = msgGeneration(PUT_REPLY,unused,0);
	msgSender(client_sd,msg_out);
	
	//wait for FILE_DATA header, need for length of data
	msgReceiver(client_sd,msg_in);
	payload=msg_in->length-12;
	//followed by data file
	if(payload>0) //!!!!!!!!!!!!!!!
	{
		int count = payload;
		//block of data deliver
		for(count=count-THROUGHPUT;count>0;count=count-THROUGHPUT){
			dataReceiver(client_sd,THROUGHPUT,buf);
			data_in(u_fd,THROUGHPUT,buf);
		}
		count = count + THROUGHPUT;
		//remain > 0 but <= THROUGHPUT
		dataReceiver(client_sd,count,buf);
		data_in(u_fd,count,buf);
	}
	//printf("Upload done\n");
}

int client_close(threadargs* client_info,struct message_s* msg_out)
{
	//printf("Client close Request\n");
	*msg_out = msgGeneration(QUIT_REPLY,unused,0);
	msgSender(client_info->client_sd,msg_out);
	//printf("close client\n");
	
	//show client sock addr.
	char* str_client_addr = inet_ntoa(client_info->client_addr.sin_addr);
	unsigned short client_port = client_info->client_addr.sin_port;
	printf("Leaved client[%d] info=> IP:%s  Port number:%hu \n", client_info->client_index,str_client_addr, client_port);	
	close(client_info->client_sd); //close client
	return 0;
}

int serviceHandler(threadargs* client_info,int client_sd, struct message_s* msg_in,struct message_s* msg_out,char* buf)
{
	int error;
	//Protocol Message wait untill get it
	//recv() func will **block** the process to wait for client's message
	int len = msgReceiver(client_sd,msg_in);
	
	//To identify the service request from the clients
	if( (memcmp(msg_in->protocol,magicNum,6)==0) && (len==12))
	{
		error = 0;
		switch((unsigned char)msg_in->type)
		{
			case LIST_REQUEST:
				listFile(client_sd,msg_out,buf);
				break;
			case GET_REQUEST:
				beDownload(client_sd,msg_in,msg_out,buf);
				break;
			case PUT_REQUEST:
				beUpload(client_sd,msg_in,msg_out,buf);
				break;
			case QUIT_REQUEST:
				return client_close(client_info,msg_out);
				break;
			default:
				error = 2;
		}
	}
	else
		error = 1;
	errorprint(error);
	if(error!=0)
		return client_close(client_info,msg_out);
	return 1;
}


void *client(void* arg)
{
	threadargs* client_info = (threadargs*)arg;
	//thread local variable
	struct message_s msg_in;
	struct message_s msg_out;
	char buf[THROUGHPUT]; //no INT_MAX , ortherwise stackoverflow -> segmentation fault 
	//--------------------
	
	connection(client_info,&msg_in,&msg_out);
	
	FILE* access_file;
	if((access_file = fopen("access.txt","r"))==NULL){
		printf("open auth. file erro: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	while(authentication(client_info->client_sd,access_file,&msg_in,&msg_out,buf)){;}
	fclose(access_file);
	
	while(serviceHandler(client_info,client_info->client_sd,&msg_in,&msg_out,buf)){;}
	
	pthread_mutex_lock(&mutex);
	//-----------------mutual exclusion for index and available[]----------------------------
	available[client_info->client_index] = 0;
	if(m>=MAX)
		pthread_cond_signal(&full);
	m--;
	//-----------------mutex end----------------------------------
	pthread_mutex_unlock(&mutex);
	
	pthread_exit(NULL);//kill the thread here!
}

//These socket options can be set by using setsockopt(2) and read with getsockopt(2) with the socket level set to SOL_SOCKET for all sockets: 
int main(int argc, char** argv){
	int client_sd;
	int sd;
	unsigned short port;
	if(argc != 2)
	{
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}
	
	port = atoi(argv[1]);
	
	//server sockect creation
	sd=socket(AF_INET,SOCK_STREAM,0);
	//reusable port setup
	long val=1;
	if(setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(long))==-1)
	{
		perror("setsockopt");
		exit(1);
	}
	//declare the address length of client
	int addr_len;

	//declare socket address for IPv4 address and port number
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;

	//server address setup
	memset(&server_addr,0,sizeof(server_addr));	//Clear
	server_addr.sin_family=AF_INET;			//IPv4
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);	//Any IP address  //inet_addr(IP);????????????
	server_addr.sin_port=htons(port);		//Port number
	
	//bind server socket to corresponding address.
	if(bind(sd,(struct sockaddr *) &server_addr,sizeof(server_addr))<0){
		printf("bind error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	
	//set server socket to listen *limited by 10 ????????????????????
	if(listen(sd,10)<0){
		printf("listen error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}

	//accecpt() func will **block** the process to wait for client's connection request
	//The job of main thread!
	int private;
	int i=0;
	m=0;
	for(i=0;i<MAX;i++)
		available[i]=0;

	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&full,NULL);
	printf("---------------Server Listen-----------------------\n");
	while(1){
	
		pthread_mutex_lock(&mutex);
		if(m>=MAX)
		{
			printf("Main thread signal: Not enough vacancy! Stop accepting.\n");
			pthread_cond_wait(&full, &mutex);	// as a result of full condition , then wait and unlock mutual exclusion.
			printf("Main thread signal: Release\n");
		}
		pthread_mutex_unlock(&mutex);
		
		addr_len = sizeof(client_addr);	//initialize it first!!!
		memset(&client_addr,0,sizeof(client_addr));
		if((client_sd = accept(sd,(struct sockaddr *) &client_addr,&addr_len))<0){
			printf("accept erro: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		pthread_mutex_lock(&mutex);
		//____________________mutual exclusion for available[] , m___________________________
        
		//find the available vacancy
		for(i=0;i<MAX;i++){
			if(available[i]==0)
			{
				ind = i;
				available[i] = 1;
				m++;
				break;
			}
		}
		//_______________________mutex end_______________________________________________
		pthread_mutex_unlock(&mutex);
		
		//after wakeup, index is assigned a value(not -1) in a rece condition way by clients'closing
		
		//args[] is chosen for come-in client
		
		pthread_mutex_lock(&mutex);
		//-----------------------mutual exclusion for index--------------------
		//args initialization
		available[ind] = 1;
		args[ind].client_index = ind;
		args[ind].client_sd = client_sd;
		args[ind].client_addr = client_addr;
		//thread creation
		if(pthread_create(&(args[ind].client_ID),NULL,client,&args[ind]))
		{
			printf("Error: pthread_create()\n");
			exit(0);
		}
		//------------------------mutex end--------------------------------------
		pthread_mutex_unlock(&mutex);
	 }
	//server closing
	close(sd);
	return 0;
}
