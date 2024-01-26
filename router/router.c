#include <stdio.h>
#include <stdlib.h>
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
#include <dirent.h>
#include <limits.h>
# include <pthread.h>
#include <netdb.h>         /* gai_strerror, getaddrinfo */
#include <utime.h>
#include <crypt.h>
#include <time.h>
#include "common.h"
//error-handling macro
#define show_error(call, err) do { errno=err; perror(call); exit(1); } while (0)
#define MAX 100 //Maximum number of thread !!!!!!!!!!

typedef struct threadargs{
	pthread_t client_ID;
	int client_index;
	int client_sd;
	struct sockaddr_in client_addr;
}threadargs;
//-----Threads' private-------
threadargs args[MAX];
//----Shared Objects-----
int available2[MAX];
int ind;
int m;
//-----------------------
//-----Semaphores--------
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // for index and available
pthread_cond_t 	full	= PTHREAD_COND_INITIALIZER;	// vacancy is full
//-----------------------
typedef struct Router
{
	in_addr_t ip; //comparison problem???
	unsigned int port;
	int router_sd;
	int RID;
	int linkCost;	//-1 for not its neighbor
}Router;

struct Entry{
	int dest_RID;
	int nextHop_RID;
	int pathCost;
}__attribute__ ((packed));

//The unique id and index of the router process
int ownRID;
int ownIndex;
//----------------------

//number
int numRouter;
int numDV;

//ALl routers' info
int* connected;
Router* router_set; //excluding itself 
//Distance Table of the router
int** DVTable;  			// -1 for NOT exist, 0 for path to itself!!!!
//routing table of router RID
int* available;
struct Entry* routing_table;

int RID_to_Index(int RID)
{
	int i;
	for(i=0;i<numRouter;i++)
		if(router_set[i].RID==RID)
			return i;
	printf("error:RID_to_Index()\n");
	return -1;
}

int index_to_RID(int index)
{
	if(index<numRouter && index >= 0)
		return router_set[index].RID;
	printf("error:Index_to_RID()\n");
	return -1;
}
void setNextHop(int d_index,int nextHop_index,int pathCost)
{
	available[d_index]=1;
	routing_table[d_index].dest_RID = index_to_RID(d_index);
	routing_table[d_index].nextHop_RID = index_to_RID(nextHop_index);
	routing_table[d_index].pathCost = pathCost;
}
int mapTopology(char* fname)
{
	FILE *fd;
	int numEdge;
	int s_id,d_id,linkCost;
	if((fd = fopen(fname, "r")) == NULL) 
	{
		if (errno == ENOENT) {
			printf("File doesn't exist\n");
			exit(0);
		}
		else{
		// Check for other errors
			printf("open():Some other error occured\n");
			exit(0);
		}
	}
	fscanf(fd,"%d",&numEdge);		
	int i,s_i,d_i;
	for(i=0;i<numEdge;i++)
	{
		fscanf(fd,"%d,%d,%d",&s_id,&d_id,&linkCost);
		//init DVTable
		s_i = RID_to_Index(s_id);
		d_i = RID_to_Index(d_id);
		if(s_id==ownRID)
		{
			DVTable[s_i][d_i] = linkCost;
			//init routing table
			if(linkCost!=-1)
				setNextHop(d_i,d_i,linkCost);
			router_set[d_i].linkCost = linkCost;
		}
	}
	return 1;
}

void displayDV()
{
	int i,j;
	for(i=0;i<numRouter;i++)
	{
		printf("%d| ",router_set[i].RID);
		for(j=0;j<numRouter;j++)
			printf("%d ",DVTable[i][j]);
		printf("\n");
	}
}
int mapRLocation(char* fname)
{
	FILE *fd;
	int IP[4];
	char domain[100] = {}; 
	char temp[100] = {};
	if((fd = fopen(fname, "r")) == NULL) 
	{
		if (errno == ENOENT)
		{
			printf("File doesn't exist\n");
			exit(0);
		}
		else
		{
			// Check for other errors
			printf("open():Some other error occured\n");
			exit(0);
		}
	}
	
	fscanf(fd,"%d",&numRouter);
	printf("r:%d",numRouter);
	
	//-------Data Structure Malloc-----------------
	router_set = malloc(sizeof(Router)*numRouter);
	connected = malloc(sizeof(int)*numRouter);
	routing_table = malloc(sizeof(struct Entry)*numRouter);
	available = malloc(sizeof(int)*numRouter);
	DVTable = malloc(sizeof(int*)*numRouter);
	
	int i,j;
	//initialization
	for(i=0;i<numRouter;i++)
	{
		connected[i] = 0;
		available[i] = 0;
		router_set[i].linkCost = -1;
		DVTable[i] = malloc(sizeof(int)*numRouter);
		for(j=0;j<numRouter;j++)
		{
			if(i==j)
				DVTable[i][j] = 0;
			else
				DVTable[i][j] = -1;
		}
	}
	//---------------------------------------------
	//Router Loaction setup
	for(i=0;i<numRouter;i++)
	{
		fscanf(fd,"%d.%d.%d.%d,%d,%d",&IP[0],&IP[1],&IP[2],&IP[3],&router_set[i].port,&router_set[i].RID);
		for(j=0;j<4;j++)
		{
			sprintf(temp,"%d",IP[j]);
			if(j!=3)
				strcat(temp,".");
			strcat(domain,temp);
		}
		printf("[%s],%hd,%d\n",domain,router_set[i].port,router_set[i].RID);
		router_set[i].ip = inet_addr(domain);
		if(router_set[i].RID==ownRID) //set ownIndex
			ownIndex = i;
		strcpy(domain,""); //reset it
	}
	return 1;
}

void gen_RTable(char* buf)
{
	char temp[100];
	int d_index;
	strcpy(buf,""); //clear
	for(d_index=0;d_index<numRouter;d_index++)
	{
		if(available[d_index]==1)
		{
		sprintf(temp,"%d",routing_table[d_index].dest_RID);
		strcat(temp," ");
		strcat(buf,temp);
		sprintf(temp,"%d",routing_table[d_index].nextHop_RID);
		strcat(temp," ");
		strcat(buf,temp);
		sprintf(temp,"%d",routing_table[d_index].pathCost);
		strcat(temp," ");
		strcat(buf,temp);
		strcat(buf,"\n");
		}
	}
}

int min_DV(int d_index,int* min)
{
	int cost;
	int nextHop;
	int NH = -1;
	int once=0;
	*min = -1;  //is there any case that no path to dest,yes
	for(nextHop=0;nextHop<numRouter;nextHop++)
	{
		//if it is not parent router itself 
		//and immediary is its neighbor (link exits)
		//and the path from immediary to dest exits
		//otherwise, To next router
		if(nextHop!=ownIndex && router_set[nextHop].linkCost!= -1 && DVTable[nextHop][d_index] != -1)
		{
			if(!once)
			{
				//Then, the link cost from parent router to immediary(index) + DV from immediary to dest(d_index).
				*min = router_set[nextHop].linkCost + DVTable[nextHop][d_index];
				NH = nextHop;
				once = 1;
			}
			else
			{
				cost = router_set[nextHop].linkCost + DVTable[nextHop][d_index];
				//if the cost is less than min, 
				//then set the new (To dest) nextHop and new path cost in the routing table
				if(cost<*min)
				{
					*min = cost;
					NH = nextHop;
				}
			}
		}
	}
	if(*min>=1000*10) //it DV is too large, then -1 
		*min=-1;
	if(*min!=-1)
		return NH;
	else
		return -1;
}

int recompute()
{
	int d_index,min,nextHop;
	int ischange = 0;
	for(d_index=0;d_index<numRouter;d_index++)
	{
		if(d_index!=ownIndex)
		{
			nextHop = min_DV(d_index,&min);
			if(min!=DVTable[ownIndex][d_index])
			{
				//update Distance table
				DVTable[ownIndex][d_index] = min;
				//update routing table
				if(nextHop != -1)
					setNextHop(d_index,nextHop,min);
				ischange = 1;
			}
		}
	}
	printf("-----After Recomputing---\n");
	displayDV();
	printf("--------------------------\n");
	return ischange;
}
int connectNeighbor(in_addr_t ip, unsigned short port)
{
	int fd;
	struct sockaddr_in addr;   //server'address = ip + port (above)
	unsigned int addrlen = sizeof(struct sockaddr_in); 

	fd = socket(AF_INET, SOCK_STREAM, 0);

	if(fd == -1)
	{
		perror("socket()");
		return -1;
	}

	memset(&addr, sizeof(struct sockaddr_in), 0);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip;    //no need to convert // already in NB order
	addr.sin_port = htons(port);

	if( connect(fd, (struct sockaddr *) &addr, addrlen) == -1 )
	{
		perror("connect()");
		return -1;
	}
	return fd;	
}

int connection(struct message_s* msg_out)
{
	int i;
	for(i=0;i<numRouter;i++)
	{
		if(router_set[i].linkCost!=-1)
		{
			if(connected[i] == 0)
			{
				if((router_set[i].router_sd = connectNeighbor(router_set[i].ip,router_set[i].port))==-1)
					return 0;
				connected[i] = 1;
			}
		}
		else
		{
			if(connected[i]==1)
			{
				*msg_out = msgGeneration(CLOSE,ownRID,-1,-1,0);
				msgSender(router_set[i].router_sd,msg_out);
				close(router_set[i].router_sd);
				connected[i] = 0;
			}
		}
	}
	return 1;
}

void dv_func(struct message_s* msg_out){
	int i;
	connection(msg_out);
	int* temp = malloc(sizeof(int)*numRouter);
	//host to network byte order
	for(i=0;i<numRouter;i++)
		temp[i] = htonl(DVTable[ownIndex][i]);
	printf("=>Notify neighbors: ");
	for(i=0;i<numRouter;i++)
	{
		if(router_set[i].linkCost!=-1)
		{
		printf("[%d],",router_set[i].RID);
		*msg_out = msgGeneration(DVMSG,ownRID,-1,-1,numRouter*sizeof(int));
		msgSender(router_set[i].router_sd,msg_out);
		//send its own DVs to neightbor i.
		dataSender(router_set[i].router_sd,numRouter*sizeof(int),(void*)temp);
		}
	}
	printf(".\n");
	free(temp);
}

void update_func(int s_id,int d_id,int weight)
{
	if(weight == 1000) //too large , then consider it as disconnected.
		weight = -1; 
	router_set[RID_to_Index(d_id)].linkCost = weight;
	printf("Link Cost Update: from itself[%d] to [%d]: %d\n",ownRID,d_id,weight);
	recompute(); //recompute DV table, but don't propagate them
}

void show_func(int agent_sd,struct message_s* msg_out,char* buf)
{
	*msg_out = msgGeneration(RESPONSE,ownRID,-1,numDV,strlen(buf)+1);
	msgSender(agent_sd,msg_out);
	gen_RTable(buf);
	dataSender(agent_sd,strlen(buf)+1/*1 for '\0'*/,(void*)buf);
	printf("----Routing table sent----\n");
	printf("%s",buf);
	printf("numDV: %d\n",numDV);
	printf("--------------------------\n");
}

void reset_func()
{
	printf("\n=>numDV Reset\n");
	numDV = 0;
}

int agent(int agent_sd,struct message_s* msg_in,char* buf)
{
		switch((unsigned char)msg_in->type)
		{
			case DV: // The role of notifying its neightbors with its changed DV
				dv_func(msg_in);
				return 1;
			case UPDATE: // Lead to recompute();
				update_func(msg_in->source,msg_in->dest,msg_in->weight);
				return 1;
			case SHOW: //respond to agent
				show_func(agent_sd,msg_in,buf);
				return 1;
			case RESET:
				reset_func();
				return 1;
			default:
				return 0;
		}
	return 0;
}

void routing(int client_sd,struct message_s* msg_out,int* DVRow,int size,int n_RID)
{
	int i;
	//receive DVs of n_RID
	dataReceiver(client_sd,size,(void*)DVRow);
	//increment the numDV
	numDV++;
	//network to host byte order
	for(i=0;i<numRouter;i++)
		DVRow[i] = ntohl(DVRow[i]);
		
	memcpy(DVTable[RID_to_Index(n_RID)],DVRow,size);	//update DVs of n_RID
	printf("---After receiving DVMSG from RID [%d]---\n",n_RID);
	displayDV();
	printf("------------------------------------------\n");
	if(recompute()) // if change
		dv_func(msg_out); //then notify its neightbor
}

int request_response(threadargs* client_info,struct message_s* msg_in,char* buf,int* DVRow)
{
	int c =1;
	int len = msgReceiver(client_info->client_sd,msg_in);
	//one message by one message to run using by mutex
	pthread_mutex_lock(&mutex);
	if( (memcmp((*msg_in).protocol,magicNum,3)==0) && (len==sizeof(struct message_s)) )
	{
		switch((unsigned char)msg_in->type)
		{
			case DVMSG:
				routing(client_info->client_sd,msg_in,DVRow,msg_in->payload,msg_in->source);
				c = 1;
				break;
			case CLOSE:
				c = 0;
				break;
			default:
				c = agent(client_info->client_sd,msg_in,buf);
		}
	}
	else
		c = 0;
	pthread_mutex_unlock(&mutex);
	return c;
}

int client_close(threadargs* client_info)
{	
	//show client sock addr.
	char* str_client_addr = inet_ntoa(client_info->client_addr.sin_addr);
	unsigned short client_port = client_info->client_addr.sin_port;
	printf("Leaved client[%d] info=> IP:%s  Port number:%hu \n", client_info->client_index,str_client_addr, client_port);		
	close(client_info->client_sd); //close client
	return 0;
}

int serviceHandler(threadargs* client_info)
{
	//thread local variable
	struct message_s msg_in;
	int* DVRow = malloc(sizeof(int)*numRouter);
	char* buf = malloc(100000); //no INT_MAX , ortherwise stackoverflow -> segmentation fault 
	//-------------------
	while(request_response(client_info,&msg_in,buf,DVRow));  //request-reponse
	client_close(client_info);
	//free
	free(buf);
	free(DVRow);
	return 1;
}

void cleanup(void *thread_num) {
  printf("Thread %d cleans up.\n", (int)thread_num);
}

void *client(void* arg)
{
	/* Register a cleanup routine for current thread, passing 1 in as argument */
	pthread_cleanup_push(cleanup, (void*)1);
	threadargs* client_info = (threadargs*)arg;
	
	//-----------Show client connection info----------------------
	char* str_client_addr = inet_ntoa(client_info->client_addr.sin_addr);
	unsigned short client_port = client_info->client_addr.sin_port;
	printf("Connected client[%d] info=> IP:%s  Port number:%hu \n", client_info->client_index,str_client_addr, client_port);
	//-------------------------------------------------------------

	//________service_________
	serviceHandler(client_info);
	//________service_________
		
	pthread_mutex_lock(&mutex);
	//-----------------mutual exclusion for index and available[]----------------------------
	available2[client_info->client_index] = 0;
	if(m>=MAX)
		pthread_cond_signal(&full);
	m--;
	//-----------------mutex end----------------------------------
	pthread_mutex_unlock(&mutex);
	
	pthread_exit(NULL);//kill the thread here!

	/* push and pop MUST be matched in the same scope */
  	pthread_cleanup_pop(1);
	return NULL;
}
int main(int argc, char** argv)
{
	//init;
	numDV = 0;
	numRouter = 0;
	
	if(argc != 4)
	{
		fprintf(stderr, "Usage: %s <router location file> <topology conf file> <router_id>\n", argv[0]);
		exit(1);
	}
	ownRID = atoi(argv[3]);
	mapRLocation(argv[1]);
	mapTopology(argv[2]);
	printf("------Initial status---------\n");
	displayDV();
	printf("-----------------------------\n");
	//--------------------server part------------------------
	int client_sd;
	int sd;
	unsigned short port = router_set[ownIndex].port;
	
	//-------attribute for detached thread Creation----------------
	 pthread_attr_t attr;
  	// Set the attributes to create threads as detached 
  	int ret = pthread_attr_init(&attr);
  	if (ret) { show_error("pthread_attr_init", ret); }
  	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  	if (ret) { show_error("pthread_attr_setdetachstate", ret); }
	//-------------------------------------------------------------

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
	{
		available2[i]=0;
	}

	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&full,NULL);
	
	printf("---------------Router %d Listen-----------------------\n",ownRID);
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
			if(available2[i]==0)
			{
				ind = i;
				available2[i] = 1;
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
		available2[ind] = 1;
		args[ind].client_index = ind;
		args[ind].client_sd = client_sd;
		args[ind].client_addr = client_addr;
		//thread creation
		if(pthread_create(&(args[ind].client_ID),&attr,client,&args[ind]))
		{
			printf("Error: pthread_create()\n");
			exit(0);
		}
		//------------------------mutex end--------------------------------------
		pthread_mutex_unlock(&mutex);
	 }
	 //connected router closing
	 for(i=0;i<numRouter;i++)
	 {
		if(connected[i]==1)
		{
			close(router_set[i].router_sd);
			connected[i]=0;
		}
	 }
	 //malloc here
	 free(routing_table);
	 free(router_set);
	 free(available);
	 free(connected);
	 for(i=0;i<numRouter;i++)
		free(DVTable[i]);
	free(DVTable);
	//server closing
	close(sd);
	return 0;
}
