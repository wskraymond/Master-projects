#define _XOPEN_SOURCE 700  /* Needed for strptime().
                            * Remember to define this before the include's. */
#define _BSD_SOURCE     /* Needed for timegm() */
#define _GNU_SOURCE
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
//error-handling macro
#define show_error(call, err) do { errno=err; perror(call); exit(1); } while (0)
//For http protocol
#define MAX 100 //Maximum number of thread !!!!!!!!!!
#define THROUGHPUT 1000
#define HEADER_MAX 6*1024 // set to 6kB 
#define S_SET_SIZE 5 // the max no. of connected servers for a client

int milestone; 
typedef struct threadargs{
	pthread_t client_ID;
	int client_index;
	int client_sd;
	struct sockaddr_in client_addr;
}threadargs;

typedef struct servers{
	in_addr_t ip; //comparison problem???
	unsigned int port;
	int server_sd;
}servers;

//-----Threads' private-------
threadargs args[MAX];
//server connection
servers server_set[MAX][S_SET_SIZE];
int server_avail[MAX][S_SET_SIZE];
int server_num[MAX]; //no. of connected servers for a client
//----Shared Objects-----
int available[MAX];
int ind;
int m;
//-----------------------
//-----Semaphores--------
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // for index and available
pthread_cond_t 	full	= PTHREAD_COND_INITIALIZER;	// vacancy is full
//-----------------------
//return the size of header received
long msgReceiver(int client_sd,char* msg_in,char* buf)
{
	long len=0;
	long recvlength=0;
	char* substr;
	long leading;
	//Clear msg!!!
	memset(msg_in,0,HEADER_MAX);
	//one byte retrieving -> better ???? time-consuming?
	while(recvlength<HEADER_MAX && strstr(msg_in,"\r\n\r\n") == NULL){
		if((len=read(client_sd,msg_in+recvlength,HEADER_MAX-recvlength))<0){
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
		}
		if(len==0)
		{
			if(milestone>1)
				return -2;
			else{
				printf("\nerror: len = 0!!!!!!!\n");
				return -1;
			}
		}
		printf("recv...\n");
		recvlength+=len;
	}
	
	//pre-data receving
	//buf clear
	memset(buf,0,sizeof(buf));
	//extract the content from data string
	if( !(substr = strstr(msg_in,"\r\n\r\n")) )
	{
		printf("header storage overflow!\n");
		exit(0);
	}
	substr += 4; //points to a byte after "\r\n\r\n"
	leading = (long)(recvlength - (substr - msg_in));
	//memcpy() for binary content!!!!!!!!!!.
	memcpy(buf,substr,leading); // copy the leading content based on recvlength-header_size.
	*substr = '\0';
	
	printf("recvlength: %ld\n",recvlength);
	printf("bytes received for header: %ld\n",strlen(msg_in));
	printf("header: %s|\n",msg_in);
	printf("bytes received for content: %ld\n",leading);
	printf("leading content: %s|\n",buf);
	
	return leading;
}
//!!!!!!!!!!!!!!!!!!!!!!!!put while loop!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
long msgSender(int client_sd,char* msg_out)
{
	long len = 0;
	long sentlength =0 ;
	//what about '\0', or use header_size
	printf("string length: %d\n",strlen(msg_out));
	while(sentlength<strlen(msg_out)){
		if((len=send(client_sd,msg_out+sentlength,strlen(msg_out)-sentlength,MSG_NOSIGNAL))<0){
			if(errno==32)
				return -1;
			printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		if(len==0)
		{
			printf("\nerror: len = 0!!!!!!!\n");
			return -1;
		}
		sentlength += len;
	}
	printf("bytes sent: %ld\n",sentlength);
	printf("header: %s|\n",msg_out);
	return sentlength;
}


//return size of content received (including leading)
long dataReceiver(int client_sd,long contentlength,char* buf)
{
	long len;
	long recvlength=0;
	while(recvlength<contentlength){
		if((len=read(client_sd,buf+recvlength,contentlength-recvlength))<0){
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
		}
		if(len==0)
		{
			printf("\nerror: len = 0!!!!!!!\n");
			return -1;
		}
		printf("recv...\n");
		recvlength+=len;
	}
	printf("remaining bytes received: %ld\n",recvlength);
	return recvlength;
}

long dataSender(int client_sd,long size,char* buf)
{
	long len;
	long sentlength=0;
	while(sentlength<size){
		if((len=send(client_sd,buf+sentlength,size-sentlength,MSG_NOSIGNAL))<0){
			if(errno==32)
				return -1;
			printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		if(len==0)
		{
			printf("\nerror: len = 0!!!!!!!\n");
			return -1;
		}
		sentlength += len;
	}
	printf("bytes sent: %ld\n",sentlength);
	return sentlength;
}

int isItem(char* msg_in,char* item)
{
	if(!strstr(msg_in,item))
		return 0;
	return 1;
}

//problem if item is at last statement ? e.g GET www.cse.cuhkhk/GET version or only get the first one?
//assume ok!!!!! 
int getValue(char* msg_in,char* start_item,char* end_item,char* value)
{
	int length;	
	char* start_s;
	char* end_s;
	if( !(start_s = strstr(msg_in,start_item)) )
	{
		printf("no this item!\n");
		return 0;
	}
	
	start_s += strlen(start_item); //points to  a pos after a space of item
	if(! (end_s = strstr(start_s,end_item)) )
	{
		printf("logical error\n");
		return 0;
	}
	if( !(length = end_s - start_s) )
	{
		printf("logical error\n");
		return 0;
	}
	
	strncpy(value,start_s,length);
	*(value + length) = '\0'; // for string handlling
	return 1;
}

//beware itme should including a trailing space
//NOt work with above, lenght differences
int modifyValue(char* msg_in,char* start_item,char* end_item,char* value)
{
	int length;	
	char* start_s;
	char* end_s;
	char temp[1000];
	if( !(start_s = strstr(msg_in,start_item)) )
	{
		printf("no this item!\n");
		return 0;
	}
	
	start_s += strlen(start_item); //points to  a pos after a space of item
	if(! (end_s = strstr(start_s,end_item)) )
	{
		printf("logical error\n");
		return 0;
	}
	if( !(length = end_s - start_s) )
	{
		printf("logical error\n");
		return 0;
	}

	strcpy(temp,end_s);	//restore the trailing one
	//printf("[%s]\n",temp);
	strcpy(start_s,value); //replace the previous one with value
	//printf("[%s]\n",start_S);	
	strcat(start_s,temp);

	return 1;	
}


int insertValue(char* msg_in,char* item,char* value)
{	
	char* start_s;
	char temp[1000] = "\r\n";
	strcat(temp,item);
	strcat(temp,value);
	strcat(temp,"\r\n\r\n");
	if( !(start_s = strstr(msg_in,"\r\n\r\n")) )
	{
		printf("no this item!\n");
		return 0;
	}
	*start_s = '\0';
	strcpy(start_s,temp);
	return 1;	
}
int connectServer(int client_index,in_addr_t ip, unsigned short port,int persistent)
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
int readAvailable(int sockfd)
{
	int ret = 0;
	struct timeval tv = {0};
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);

	ret = select(sockfd+1, &readfds, NULL , NULL, &tv);  //select system call!!!!
	if (ret == -1) {
		perror("select");
		return 2; // bad file descriptor !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}
	else if (FD_ISSET(sockfd, &readfds))
	{
		return 1;
	} 
	return 0;
}
int DNS(int client_index,char* domain,char* port,int persistent)
{
   int server_sd;
  /* Resolve domain name */
  struct addrinfo hints, *results;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;        /* IPv4 */
  hints.ai_socktype = SOCK_STREAM;  /* TCP connection */
  hints.ai_flags = 0;
  hints.ai_protocol = 0;
  int ret = getaddrinfo(domain, port, &hints, &results);

  /* Error-handling */
  if (ret != 0) {
    if (ret == EAI_SYSTEM) {
      perror("getaddrinfo");
    } else {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    }
    return -1;
  }
  /* List results */
  struct addrinfo *p_results = results;
  //there may be many Host IP for this host name , e.g yahoo.com but just one for www.yahoo.com
  //attempt to connect one of them.
  int i;
  for (i=0; p_results!=NULL; p_results=p_results->ai_next) {
    struct sockaddr_in *sa = (struct sockaddr_in *)p_results->ai_addr;
	if(milestone>1)
	{
			long i,count;
			for(i=0,count=0;i<S_SET_SIZE && count<server_num[client_index]; i++)
			{
				if(server_avail[client_index][i]==1)
				{
					if(server_set[client_index][i].ip == inet_addr(inet_ntoa(sa->sin_addr))
						&&server_set[client_index][i].port == ntohs(sa->sin_port) )
					{
						int error;
						if ((error = readAvailable(server_set[client_index][i].server_sd))>0) {
							printf("client[%d]:=>Resue Failure! [%s:%hd] web server close the connection.\n", client_index, inet_ntoa(sa->sin_addr), ntohs(sa->sin_port)); //print out one, I
							if(error!=2) //!!!!!!!!!!!!!!!!!!
								close(server_set[client_index][i].server_sd); //close proxy-server conn.
							server_avail[client_index][i] = 0; //reset avail
							server_num[client_index]--; 
							break;
						} else {
							printf("client[%d]: have reuse connection of [%s:%hd] web server\n", client_index, inet_ntoa(sa->sin_addr), ntohs(sa->sin_port)); //print out one, I
							return server_set[client_index][i].server_sd;
						}
					}
					count++;
				}
			}
	}
	
	//if not mile 1 or no server for reuse
	//then connect to
	
    if( (server_sd = connectServer(client_index,inet_addr(inet_ntoa(sa->sin_addr)), ntohs(sa->sin_port),persistent)) != -1)
    {
		printf("client[%d]: have connected to %s:%hd web server\n", client_index, inet_ntoa(sa->sin_addr), ntohs(sa->sin_port)); //print out one, IP and port
		if(milestone>1)
		{
			int i;
			for(i=0;i<S_SET_SIZE;i++)
			{
				if(server_avail[client_index][i]==0)
				{
					server_set[client_index][i].ip = inet_addr(inet_ntoa(sa->sin_addr));
					server_set[client_index][i].port = ntohs(sa->sin_port);
					server_set[client_index][i].server_sd = server_sd;
					server_avail[client_index][i] = 1;
					server_num[i]++;				
				}
			}
		}
		break;
    }
    i++;
  }
  freeaddrinfo(results);
    return server_sd;
}

int client_close(threadargs* client_info)
{	
	//show client sock addr.
	char* str_client_addr = inet_ntoa(client_info->client_addr.sin_addr);
	unsigned short client_port = client_info->client_addr.sin_port;
	printf("Leaved client[%d] info=> IP:%s  Port number:%hu \n", client_info->client_index,str_client_addr, client_port);	

	if(milestone>1)
	{
		long i,count;
		for(i=0,count=0;i<S_SET_SIZE && count<server_num[client_info->client_index]; i++)
		{
			if(server_avail[client_info->client_index][i]==1)
			{
				close(server_set[client_info->client_index][i].server_sd);
				server_avail[client_info->client_index][i]=0;
				count++;
			}
		}
		server_num[client_info->client_index] = 0;
	}
		
	close(client_info->client_sd); //close client
	return 0;
}

int data_out(int d_fd,long size,char* buf)
{
	long len;
	//memset(buf,0,size);
	if((len=read(d_fd,buf,size))<0){
		printf("read error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	*(buf+size)= '\0';
	return len;
}

int data_in(int u_fd,long size,char* buf)
{
	long len;
    if((len=write(u_fd,buf,size))==-1){
		printf("write error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	//printf("data_in: %s\n", buf);
	return len;
}

int cache_out(int client_sd,char* file_c,char* buf)
{
			//set path
			char path[40] = "cache/";
			strcat(path,file_c);
			long payload = 0;
			int d_fd;
			if( (d_fd = open(path,O_RDONLY,S_IRWXU)) == -1)
			{
				printf("open d_file erro: %s (Errno:%d)\n",strerror(errno),errno);
				exit(0);
			}
			//payload setting // check file size
			struct stat s_file;
			if(fstat(d_fd,&s_file)==-1)
			{
				printf("file status erro: %s (Errno:%d)\n",strerror(errno),errno);
				exit(0);
			}
			payload = s_file.st_size;
			
	//--------------mutex for directory-----------------------
	pthread_mutex_lock(&mutex);
	if(payload>0) //!!!!!!!!!!!!!!!
	{
		//buffering
		long count = payload;
		//block of data deliver
		for(count=count-THROUGHPUT;count>0;count=count-THROUGHPUT){
			data_out(d_fd,THROUGHPUT,buf);
			dataSender(client_sd,THROUGHPUT,buf);
		}
		count = count + THROUGHPUT;
		//remain > 0 but <= THROUGHPUT
		data_out(d_fd,count,buf);
		dataSender(client_sd,count,buf);	
		printf("================>file cache out\n");
	}
	close(d_fd);
	pthread_mutex_unlock(&mutex);
	//--------------mutex END-----------------------
	return 1;	
}

//update time(date) and use the corresponing old one
void getCurrentTime(char* result){
	struct tm gmttm;
	time_t local;
	time(&local);
	gmtime_r(&local,&gmttm);
	strftime(result,50,"%a, %d %b %Y %H:%M:%S GMT",&gmttm);
}
int msgGenerator(char* msg_in,char* path,int status)
{
	int h_fd;
	long size;
	char result[50]={0};
	getCurrentTime(result);
	if(status==200)
	{
		//get the old one as template
		pthread_mutex_lock(&mutex); //------------------->mutex for dir
		if( (h_fd = open(path,O_RDONLY,S_IRWXU)) == -1)
		{
			printf("path: %s\n",path);
			printf("open [header file] erro: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		//payload setting  // check file size
		struct stat s_file;
		if(fstat(h_fd,&s_file)==-1)
		{
			printf("file status erro: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		size = s_file.st_size;
		data_out(h_fd,size,msg_in);
		if(isItem(msg_in,"\r\nDate: "))
		{
			modifyValue(msg_in,"\r\nDate: ","\r\n",result);
		}
		close(h_fd);
		printf("==================> header file cache out for msg generation\n");
		pthread_mutex_unlock(&mutex); //------------------->mutex END
	}else if(status==304){
		char msga[]="HTTP/1.1 304 Not Modified\r\n";
		char msgb[]="Date: ";
		char msgd[]="\r\n";
		char msge[]="Server: Apache\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\n\r\n";
		strcpy(msg_in,msga);
		strcat(msg_in,msgb);
		strcat(msg_in,result);
		strcat(msg_in,msgd);
		strcat(msg_in,msge);
		printf("==================> response header gen out for msg generation\n");
	}
	else{
		printf("Error: msgGenerator()!\n");
		exit(0);
	}
	return 1;
}

struct request{
	int toServer;
	int extension;
	int status; 
	int modified;
	char lTime[50]; //http data string ,no need to set when unused
	char file_c[23]; //crypted filename ,no need to set when unused
};

struct response{
	int cache;
	int object;
	int status;
};

int checkExtension(char* url){
	if(strstr(url, ".html")|| strstr(url, ".jpg") ||strstr(url, ".gif") ||strstr(url, ".txt"))
	{
		return 1;
	}
	else
		return 0;
}
int checkFile(char* fname){
	FILE *fd;
	if((fd = fopen(fname, "r")) == NULL) 
	{
		if (errno == ENOENT) {
		//	printf("File doesn't exist\n");
			return 0;
		}
		else{
		// Check for other errors
		//	printf("open():Some other error occured\n");
			return 0;
		}
	}
	else {
		fclose(fd);
		return 1;
	}
}

void getIMT(char* path, struct request* signal){
	
	struct stat sb;
	struct tm mm;
	if(stat(path, &sb)==-1){
		printf("----fstat error-------\n\n");
		exit(0);
	}
//	printf("Last modification time of stat.c: %ld\n\n", sb.st_mtime);
   	// convert local integer time to HTTP date string (GMT) 
	gmtime_r(&sb.st_mtime, &mm);
    	char last_modified_time[50] = {0};
    	size_t len = strftime(last_modified_time, sizeof(last_modified_time),
                        "%a, %d %b %Y %H:%M:%S GMT", &mm);
	strncpy(signal->lTime,last_modified_time,50);	
}

int compareIMS(char* path, char* requestIMS){
// return 1: IMS of browser > proxy
//	 -1: 		    <	
//        0		    =

	struct stat sb;
   	struct tm tm;
	struct tm mm;
	if(stat(path, &sb)==-1){
		printf("----fstat error-------\n\n");
		exit(0);
	}
//	printf("Last modification time of stat.c: %ld\n\n", sb.st_mtime);
   	/* convert local integer time to HTTP date string (GMT) */
	gmtime_r(&sb.st_mtime, &mm);
    	char last_modified_time[50] = {0};
    	size_t len = strftime(last_modified_time, sizeof(last_modified_time),
                        "%a, %d %b %Y %H:%M:%S GMT", &mm);
  //  	printf("Or, in HTTP date format (len=%u): %s\n", len, last_modified_time);
	

//	printf("RequestIMS: %s\n",requestIMS);
	/* convert string (GMT) back to local integer time */
	strptime(requestIMS, "%a, %d %b %Y %H:%M:%S GMT", &tm);
  //  	printf("requestIMS integer time: %ld\n\n", timegm(&tm));

	if(timegm(&tm) > sb.st_mtime)
		return 1;
	else if(timegm(&tm)<sb.st_mtime)
		return -1;
	else
		return 0;
}



struct request parseRequest(char* msg_in, int mile){
	struct request signal;
	signal.status = -1;
	signal.modified = 0;
	signal.extension = 0;
	char value[1000];
	int IMS = 0;
	int CT = 0;
	int cache =-1;
	int doit =0;
	int fileExist =0;
	int i;
//	char filename[23];
	char path[30];
	char reqIMS[50];

	if(mile==3){
	strcpy(path,"cache/");

	// check control variables----------------------------------------
	if(isItem(msg_in,"\r\nIf-Modified-Since: "))
	{
		getValue(msg_in,"\r\nIf-Modified-Since: ","\r\n",value);
		IMS = 1;
		strcpy(reqIMS,value);
	//	printf("ReqIMS is : |%s|\n\n",reqIMS);
	}
	if(isItem(msg_in,"\r\nCache-Control: "))
	{	
		CT = 1;
		getValue(msg_in,"\r\nCache-Control: ","\r\n",value);
		if(isItem(value,"no-cache"))
			cache = 0;
	}
	
	if((isItem(msg_in,"GET "))){
		getValue(msg_in,"GET "," ",value);
		if(checkExtension(value)==1){
			doit = 1;
			signal.extension = 1;
			struct crypt_data data;
			data.initialized = 0;
			char* res;
			res = crypt_r(value, "$1$00$",&data);
			//decrypt the url into filename
			printf("orignal encrpted code: |%s|\n\n",res);
			res += 6;
			for(i =0; i<22; i++){
				if(res[i]=='/')
					res[i]='_';
			}
			strncpy(signal.file_c, res,23);
			printf("filename is |%s|\n\n",signal.file_c);
			strncat(path,signal.file_c,23);
			// check the file exist or not
			if(checkFile(path)==1)
			{
				fileExist =1;
				getIMT(path,&signal);
			}
		}
	}
	// end of check------------------------------------------------------------

	printf("fileExist:%d ; IMS:%d ; CT: %d ; Cache: %d; doit: %d \n\n" ,fileExist, IMS,CT, cache, doit);
	int result;
	if(fileExist == 1 && IMS == 1){
		result = compareIMS(path, reqIMS);
		getIMT(path, &signal);
		if(result ==1)
			printf("Request IMS > Last modified time of cache \n\n");
		else if(result ==-1)
			printf("Request IMS < Last modified time of cache \n\n");
		else if(result ==0)
			printf("Request IMS = Last modified time of cache \n\n");
	}

//-------------------------------------------------------------------------------------
	if(doit==0){
		signal.status = -1;
		signal.toServer = 1;
	}
	else
	{
		if(IMS==0)
		{
			if(CT==0)
			{	
				// case 1

				if(fileExist == 1)
				{
					// no If-Modified-Since + no Cache-Control ( Proxy have web object)
					signal.status = 200;
					signal.toServer = 0;
				}
				else{
					// no If-Modified-Since + no Cache-Control ( Proxy no have required web object)
					signal.status = -1;
					signal.toServer = 1;
				}
			}
			else{
				if(cache==0)
				{
					// case 3

					if(fileExist==1)
					{
						// no If-Modified-Since + Cache-Control: no-cache ( Proxy have required web object)
						signal.modified = 1;
						signal.status = -1;
						signal.toServer = 1;
						
					}
					else{
						// no If-Modified-Since + Cache-Control: no-cache ( Proxy no have required web object)
						signal.status = -1;
						signal.toServer = 1;
					}
				}
				else{
					//treat as case 1

					if(fileExist == 1)
					{	
						// no If-Modified-Since + no Cache-Control ( Proxy have web object)
						signal.status = 200;
						signal.toServer = 0;
					}
					else{
						// no If-Modified-Since + no Cache-Control ( Proxy no have required web object)
						signal.status = -1;
						signal.toServer = 1;
					}	
				}
				
			}
			
			
		}
		else{
			//IMS = 1
			if(CT==0)
			{
				//case 2
				if(fileExist == 0)
				{
					// If-Modified-Since + no Cache-Control ( Proxy no have required web object)

					signal.status = -1;
					signal.toServer = 1;
				}
				else{
					
					// If-Modified-Since + no Cache-Control ( Proxy have required web object)
					result = compareIMS(path, reqIMS);
					if(result ==  1)
					{
						// browser new than proxy
						signal.status = 304;
						signal.toServer = 0;
					}
					else if(result == -1 )
					{
						// browser older than proxy
						signal.status = 200;
						signal.toServer = 0;
					}
					else if(result == 0)
					{
						// -----------======================????========
						// browser  = proxy (IMS)
						signal.status = 304;
						signal.toServer = 0;
					}
					
				}
			}
			else{
				//CT==1
				if(cache==0)
				{
					// case 4
					// If-Modified-Since +  Cache-Control: no-cache ( Proxy have no required web object)
					if(fileExist == 0){
						signal.status = -1;
						signal.toServer = 1;
					}
					// If-Modified-Since +  Cache-Control: no-cache ( Proxy have required web object)
					else if(fileExist == 1)
					{
						result = compareIMS(path, reqIMS);
						if(result ==  1)
						{
							// browser new than proxy
							signal.status = -1;
							signal.toServer = 1;
						}
						else if(result == -1 )
						{
							// browser older than proxy
							signal.modified = -1;
							signal.status = -1;
							signal.toServer = 1;
						}
						else if(result == 0)
						{
							// -----------======================????========
							// browser  = proxy (IMS)
							signal.status = -1;
							signal.toServer = 1;
						}	
					
					}
				}
				else{
					//another cases treat as IMS + no cache control
					
					if(fileExist == 0)
					{
						signal.status = -1;
						signal.toServer = 1;
					}
					else if(fileExist == 1)
					{
						
						result = compareIMS(path, reqIMS);
						if(result ==  1)
						{
							// browser new than proxy
							signal.status = 304;
							signal.toServer = 0;
						}
						else if(result == -1 )
						{
							// browser older than proxy
							signal.status = 200;
							signal.toServer = 0;
						}
						else if(result == 0)
						{
							// browser  = proxy (IMS)
							signal.status = 304;
							signal.toServer = 0;
						}	
					
					}
					
				}
			}
		}
	}
	printf("----------------------------------------------------------------------\n");
	printf("Last MOdified Time is : |%s|\n\n",signal.lTime);
	printf("----------------------------------------------------------------------\n");
	printf("ReqIMS is : |%s|\n\n",reqIMS);
	printf("----------------------------------------------------------------------\n");
	printf("toServer: %d;  status: %d;  modified: %d; extension: %d \n\n",signal.toServer, signal.status, signal.modified,signal.extension);
	printf("----------------------------------------------------------------------\n");
	}
	else{
		signal.toServer = 1;
		signal.status = -1;
		signal.modified = 0;
	}
	return signal;
}
struct response parseResponse(char* msg_in,struct request reInfo,int mile){
	
	struct response signal;
	if(mile==3){
	if (reInfo.toServer == 1){
		if(isItem(msg_in,"200 OK\r\n"))	
		{
			if(reInfo.extension==1){
			// cache first(use request's encrpted filename) then forward it to browser
			signal.cache = 1;
			signal.object = 1;
			signal.status = -1;}
			else if(reInfo.extension==0)
			{
				signal.cache = 0;
				signal.object = 1;
				signal.status = -1;
			}
		}
		else if(isItem(msg_in, "304 Not Modified\r\n"))
		{
			if (reInfo.modified==-1 && reInfo.status==-1 && reInfo.toServer ==1)
			{
				//Proxy have the up-to-date web object
				//retrieves it and return it to browser
				//HTTP respones 200 construct by yourself
				signal.cache = 0;
				signal.object =0;
				signal.status = 200;
			}
			else{
				// client browser has cached the most up-to-date object
				// Just forward 304 HTTP reponse to browser
				signal.cache = 0;
				signal.object =0;
				signal.status = -1;
			}
		}
		else{
			// forward to browser directly
				signal.cache = 0;
				signal.object = 1;
				signal.status = -1;
			}
		}
	printf("----------------------------------------------------------------------\n");
	printf("Response's  cache: %d;	object:  %d;   status: %d \n\n",signal.cache, signal.object, signal.status);
	printf("----------------------------------------------------------------------\n");
	}
	else{
		signal.cache = 0;
		signal.object =1;
		signal.status = -1;
	}
	return signal;

}


int forwardToServer(int client_index,struct request* rt_signal,char* msg_in,char* buf){
		//DNS service => connect to web server
		//___________________________________
		int server_sd;
		char domain[200];
		char port[10] = "80";  //default one
		if(isItem(msg_in,"\r\nHost: "))
		{
			getValue(msg_in,"\r\nHost: ","\n",domain);
			if(isItem(domain,": "))
			{
				getValue(domain,": ","\r",port);
				getValue(msg_in,"\r\nHost: ",": ",domain);
			}
			else
				getValue(msg_in,"\r\nHost: ","\r\n",domain);
		}
		else
		{
			printf("\n error: host \n");
			return -1; //close connection
		}
	
		printf("\n%s %s\n",domain, port);
		if((server_sd = DNS(client_index,domain,port,0))==-1)
		{
			printf("Conn error\n");
			return -1;
		}
	//_________Fire Preperation_______________
	//---------Non-persistent connection------
	if(milestone==1)
	{
		if(isItem(msg_in,"\r\nConnection: "))
			modifyValue(msg_in,"\r\nConnection: ","\r\n","close");
		else
			insertValue(msg_in,"Connection: ","close");

		if(isItem(msg_in,"\r\nProxy-Connection: "))
			modifyValue(msg_in,"\r\nProxy-Connection: ","\r\n","close");
		else
			insertValue(msg_in,"Proxy-Connection: ","close");
	}
	//------------------------------
	
	//----------IMS time------------
	if(rt_signal->modified==1)
	{
		if(!isItem(msg_in,"\r\nIf-Modified-Since: "))
			insertValue(msg_in,"If-Modified-Since: ",rt_signal->lTime);	
		else
			printf("has IMS\n");
	}
	else if(rt_signal->modified==-1)
	{
		if(isItem(msg_in,"\r\nIf-Modified-Since: "))
			modifyValue(msg_in,"\r\nIf-Modified-Since: ","\r\n",rt_signal->lTime);
		else
			printf("no IMS\n");
	}
	//-----------------------
	msgSender(server_sd,msg_in);
	//____End of Fire Preparation________
	return server_sd;
}
int forwardToBrowser(int client_sd,struct request* rt_signal,char* msg_in,char* buf){
	//________Packet Fire_________________
	if(rt_signal->status!=-1)
	{
		//200 , 304 header is craft from old one
		//beware cached web object is a plaint file that is saved in a non-extension mode
		//since url (to crypted filename) has already included file extension
		//need to append .tmp
		
		//set path
		char path[40] = "header/";
		strcat(path,rt_signal->file_c);
		strcat(path,".tmp");
		msgGenerator(msg_in,path,rt_signal->status);
	}
	else
	{
		printf("error: forwarToBrowser\n");
		exit(0);
	}
	//---proxy to target----
	//message
	msgSender(client_sd,msg_in);
	if(rt_signal->status==200)
	{
		cache_out(client_sd,rt_signal->file_c,buf);
	}
	//________End of Packet Fire_________________
	return 1;
}

time_t convertToUnixTime(char* last_modified_time)
{
	struct tm tm;
    strptime(last_modified_time, "%a, %d %b %Y %H:%M:%S GMT", &tm);
    return timegm(&tm);
}
int header_in(char* msg_in, struct request* rt_signal)
{
		pthread_mutex_lock(&mutex); //------------------->mutex for dir
		int h_fd;
		char header[40] = "header/";
		strcat(header,rt_signal->file_c);
		strcat(header,".tmp");
		if( (h_fd = open(header,O_WRONLY|O_CREAT,S_IRWXU)) == -1)
		{
			printf("open [header file] erro: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		data_in(h_fd,strlen(msg_in),msg_in);
		close(h_fd);
		printf("==================>header file cache in\n");
		pthread_mutex_unlock(&mutex); //------------------->mutex END
		return 1;
}
int backToBrowser(threadargs* client_info,int server_sd,struct request* rt_signal, struct response* rp_signal, char* msg_in, char* buf,long leading)
{
	long payload = 0;
	char str_value[10];
	//---------Non-persistent connection----------------------------------
	if(milestone==1)
	{
		if(isItem(msg_in,"\r\nConnection: "))
			modifyValue(msg_in,"\r\nConnection: ","\r\n","close");
		else
			insertValue(msg_in,"Connection: ","close");

		if(isItem(msg_in,"\r\nProxy-Connection: "))
			modifyValue(msg_in,"\r\nProxy-Connection: ","\r\n","close");
		else
			insertValue(msg_in,"Proxy-Connection: ","close");
	}
	//--------------------------------------------------------------------
		
	if(rp_signal->status==200)
	{
		//status = 200 
		char header[40] = "header/";
		strcat(header,rt_signal->file_c);
		strcat(header,".tmp");
		//get the old one, Not current header with 304
		msgGenerator(msg_in,header,200);
	}
	
	//if caching a file ,then should cache its corresponding header as well.
	if(rp_signal->cache)
	{
		header_in(msg_in,rt_signal);
	}	
	//proxy to browser
	msgSender(client_info->client_sd,msg_in);
	
	//-----------------payload from server---------------------------------------------
	if(rp_signal->object)
	{
		if(isItem(msg_in,"\r\nContent-Length: "))
		{
			getValue(msg_in,"\r\nContent-Length: ","\r\n",str_value);
			payload = atoi(str_value);
		}
		else{
			printf("\ncontent length = 0!!\n");
			return 1;
		}
		//--------------------------------------------------------------------
		
		//---------------caching || sending--------------------------------
		//-----------------leading data send first-------------------------
		int u_fd; //for cache-in
		char path[40] = "cache/";
		dataSender(client_info->client_sd,leading,buf);
		if(rp_signal->cache)
		{	
			//set path
			strcat(path,rt_signal->file_c);
			//could overwrite it ?? if exist
			printf("path:%s\n",path);
			pthread_mutex_lock(&mutex); //------------------->mutex for dir
			if( (u_fd = open(path,O_WRONLY|O_CREAT,S_IRWXU)) == -1)
			{
				printf("open u_file erro: %s (Errno:%d)\n",strerror(errno),errno);
				exit(0);
			}
			data_in(u_fd,leading,buf);
		}
		//-----------------following data--------------------------------------
		if(payload>0) 
		{
			long count = payload - leading;
			//block of data deliver
			for(count=count-THROUGHPUT;count>0;count=count-THROUGHPUT){
				dataReceiver(server_sd,THROUGHPUT,buf);
				if(rp_signal->cache)
					data_in(u_fd,count,buf);
				dataSender(client_info->client_sd,THROUGHPUT,buf);
			}
			count = count + THROUGHPUT;
			//remain > 0 but <= THROUGHPUT
			dataReceiver(server_sd,count,buf);
			if(rp_signal->cache){
				//remaining one
				data_in(u_fd,count,buf);
				//Set Modification Time of cached file
				struct utimbuf AMT;
				char last_modified_time[50] = {0};
				if(isItem(msg_in,"\r\nLast-Modified: "))
				{
					getValue(msg_in,"\r\nLast-Modified: ","\r\n",last_modified_time);
				}
				else if(isItem(msg_in,"\r\nDate: "))
				{
					getValue(msg_in,"\r\nDate: ","\r\n",last_modified_time);
				}
				else{
					printf("no both time M&D!\n");
					exit(0);
				}
				time_t time_int= convertToUnixTime(last_modified_time);
				AMT.actime = time_int;       /* access time */
				AMT.modtime = time_int;      /* modification time */
				if(utime(path,&AMT)==-1)
				{
					printf("utime() error: %s (Errno:%d)\n",strerror(errno),errno);
					exit(0);
				}
				close(u_fd);
				printf("========================>file cache in \n");
				pthread_mutex_unlock(&mutex);//------------------->mutex END
			}
			dataSender(client_info->client_sd,count,buf);
		}
	}
	else if(rp_signal->status==200)
	{
		//payload from cached web object in proxy.
		cache_out(client_info->client_sd,rt_signal->file_c,buf);
	}
	return 1;
}

int request_response(threadargs* client_info,char* msg_in,char* buf)
{
	long leading;
	int server_sd;
	struct request rt_signal;
	struct response rp_signal;
	//________________________Browser<->Proxy<->Server____________________________________________
	
	//browser to proxy
	//--------------Persisten connection checking------------------------------- 
	if( (leading = msgReceiver(client_info->client_sd,msg_in,buf)) < 0 )
		return 0; //no response received from client // End of persisten connection
	//--------------------------------------------------------------------------
	//____________________________Parse request___________________________________
	rt_signal = parseRequest(msg_in,milestone); //only for milestone 3
	
	//To find target to Fire !
	if(rt_signal.toServer)
	{
		server_sd = forwardToServer(client_info->client_index,&rt_signal,msg_in,buf);
		if(server_sd==-1)
		{
			return 1; //conn error
		}
		//server to proxy
		leading = msgReceiver(server_sd,msg_in,buf);
		rp_signal = parseResponse(msg_in,rt_signal,milestone);
		backToBrowser(client_info,server_sd,&rt_signal,&rp_signal,msg_in,buf,leading);
	}
	else {
		forwardToBrowser(client_info->client_sd,&rt_signal,msg_in,buf);
	}
	//___________Immediate close for Non-Persistent connection_________________
	//-----------Proxy-Server connection close----------
	if(milestone==1)
		close(server_sd);
	//_________________________________________________________________________
	
	return 1;
}

int serviceHandler(threadargs* client_info)
{
	//thread local variable
	char msg_in[HEADER_MAX]; //local storage when put in thread func.
	char msg_out[HEADER_MAX];
	char buf[THROUGHPUT]; //no INT_MAX , ortherwise stackoverflow -> segmentation fault 
	//----------------------
	while(request_response(client_info,msg_in,buf));  //request-reponse pair pipelining 
	client_close(client_info); //put in cleanup ??????????
	return 1;
}

/* Cleanup after thread exits */
//a clean up routine
//it is a thread function for push and pop resource management
void cleanup(void *thread_num) {
  printf("Thread %d cleans up.\n", (int)thread_num);
	// close fd and free malloc here // defecto purpose
}

void *client(void* arg)
{
	/* Register a cleanup routine for current thread, passing 1 in as argument */
	pthread_cleanup_push(cleanup, (void *)1);

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
	available[client_info->client_index] = 0;
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
//These socket options can be set by using setsockopt(2) and read with getsockopt(2) with the socket level set to SOL_SOCKET for all sockets: 
int main(int argc, char** argv){
	signal(SIGPIPE, SIG_IGN);  //for igore the SIGPIPE termination signal when one of the remote party close connection, 
								//and proxy still sends data
	int client_sd;
	int sd;
	unsigned short port;
	
	//-------attribute for detached thread Creation----------------
	 pthread_attr_t attr;
  	/* Set the attributes to create threads as detached */
  	int ret = pthread_attr_init(&attr);
  	if (ret) { show_error("pthread_attr_init", ret); }
  	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  	if (ret) { show_error("pthread_attr_setdetachstate", ret); }
	//-------------------------------------------------------------

	if(argc != 3)
	{
		fprintf(stderr, "Usage: %s [port] [milestone]\n", argv[0]);
		exit(1);
	}
	
	port = atoi(argv[1]);
	milestone = atoi(argv[2]);
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
		available[i]=0;
		server_num[i]=0;
	}

	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&full,NULL);
	
	if(mkdir("cache",S_IRWXU)==-1)
	{
		if(errno!=EEXIST)
		{
			perror("Create cache folder error!!!!!");
			exit(0);
		}
		else
			printf("./cache found \n");
	}
	if(mkdir("header",S_IRWXU)==-1)
	{
		if(errno!=EEXIST)
		{
			perror("Create header folder error!");
			exit(0);
		}
		else
			printf("./header found \n");
	}
	printf("---------------Folders Prepared-----------------------\n");
	printf("---------------Proxy Listen-----------------------\n");
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
		if(pthread_create(&(args[ind].client_ID),&attr,client,&args[ind]))
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
