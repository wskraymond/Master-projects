#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myftp.h"
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // "struct sockaddr_in"
#include <arpa/inet.h>  // "in_addr_t"
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


enum STATES {IDLE, IS_OPEN, IS_AUTH, IS_MAIN, IS_LS, IS_PUT,IS_GET};
enum STATES state = IDLE;
int fd, len, check;
struct message_s msg_in, msg_out;
char* ls_buf;
void* dl_buf;
void* up_buf;
//------------------------------------------------
void showPath(){
	printf("Client> ");
}
  
void clean(char** token){
 	int i;
        for(i=0; i<128; i++)
           token[i]=NULL;
}
 
int getToken(char** token, char* input){
	int i;
        char* temp = strtok(input, " ");
        for(i=0; temp != NULL; i++){
        token[i] = temp;
        temp = strtok(NULL, " ");
	}
	return i;
}

void msg_limit(int length){
	if(length > INT_MAX)
	{
		printf("The length of message is larger than INT_MAX\n Close connection....\n");
		exit(0);
	}
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

/*int dataReceiver(int size)
{
	// the reason for + 12 is: avoid any unexcepted error. 
        memset(ls_buf,0,size+12);
        if((len=recv(fd,ls_buf,size,0))<0){
                printf("data receive error: %s (Errno:%d)\n", strerror(errno),errno);
                exit(0);
        }
        printf("bytes received: %d\n",len);
        return len;
}
*/
int dataReceiver(int fd,int size,char* buf)
{
        int leng=0;
        int recvlength=0;
        memset(buf,0,size+12);
        while(recvlength<size){
                if((leng=recv(fd,buf+recvlength,size-recvlength,0))<0){
                        printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
                        exit(0);
                }
                printf("recv...\n");
                recvlength+=leng;
        }
        printf("bytes received is: %d\n",leng);
        return leng;
}


int dataSender(int fd,int size,char* buf)
{
	int leng;
        int sentlength=0;
        while(sentlength<size){
                if((leng=send(fd,buf+sentlength,size-sentlength,0))<0){
                        printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
                        exit(0);
                }
                sentlength += leng;
        }
        printf("bytes sent is: %d\n",leng);

        return leng;
}


//-------------------------------------------------

void open_conn(char** token){

	in_addr_t ip;
	unsigned short port;

	struct sockaddr_in addr;
	unsigned int addrlen = sizeof(struct sockaddr_in);

	// ip convertion	
	if( (ip = inet_addr(token[1])) == -1 )
	{
		perror("inet_addr()");
		exit(1);
	}
	// port convertion
	port = atoi(token[2]);
	
	fd = socket(AF_INET, SOCK_STREAM, 0);
	
	// socket descriptor
	if(fd == -1)
	{
		perror("socket()");
		exit(1);
	}

	// message type for connection, detect server is on / off
	memset(&addr, sizeof(struct sockaddr_in), 0);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip;
	addr.sin_port = htons(port);
	if( connect(fd, (struct sockaddr *) &addr, addrlen) == -1 )
	{
		perror("connect()");
		state = IDLE;
		return;
	}

	msg_out = msgGeneration(OPEN_CONN_REQUEST,unused,0);

	
	// send connection request for targert server
	if((len=send(fd, &msg_out,sizeof(msg_out),0))<0){
			printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	
	 memset(&msg_in,0,sizeof(struct message_s));
	// receive reply from server
	if((len=recv(fd,&msg_in,sizeof(msg_in),0))<0){
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
	}


	msg_limit(len);
	// analysis the reply message server
	check = msgChecker(&msg_in, OPEN_CONN_REPLY, 1, len);
	errorprint(check);

	// if the reply message is correct, go to got authentication
	if(check == 0){
		printf("Server connection accepted. IP: %s %s\n",token[1], token[2]);
		state = IS_AUTH;
	}
	else
		state = IDLE;

}

void auth_conn(char** token){
	int len_payload;
	len_payload = strlen(token[1]) + strlen(token[2]) + 2;

	printf("length is %d\n",len_payload);
	//check the length field of message
	printf("user: %s; pw: %s\n",token[1],token[2]);
	printf("user len: %d, pw len: %d\n",strlen(token[1]),strlen(token[2]));
	msg_limit(len_payload);
	msg_out = msgGeneration(AUTH_REQUEST,unused,len_payload);
	
	// send authentication request to server
	if((len=send(fd, &msg_out,sizeof(msg_out),0))<0){
			printf("Send Authentication request Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	// send user-id 
	if((len=send(fd, token[1], strlen(token[1]), 0))<0){
			printf("Send USER ID Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	// send space 
	if((len=send(fd, " ", 1, 0))<0){
			printf("Send space Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	// send Password 
	if((len=send(fd, token[2],strlen(token[2]),0))<0){
			printf("Send Password Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	// send null-terminated 
	if((len=send(fd, "\0", 1, 0))<0){
			printf("Send null-terminated Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}

	memset(&msg_in,0,sizeof(struct message_s));
	// receive authentication reply from server
	if((len=recv(fd,&msg_in,sizeof(msg_in),0))<0){
			printf("Receive authentication reply error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
	}
	//network to host byte
	msg_in.length = ntohl(msg_in.length);	

	msg_limit(msg_in.length-12);

	// analysis the reply message server
	check = msgChecker(&msg_in, AUTH_REPLY, 1, len);
	errorprint(check);

	if(check==0){	
		printf("Authentication granted.\n");
		state = IS_MAIN;
	}
	else{
		printf("ERROR: Authentication rejected. Connection closed.\n");
		state = IDLE;
	}
}

void list_file(){
	
	msg_out = msgGeneration(LIST_REQUEST,unused,0);
	
	// send list_ request to server
	if((len=send(fd, &msg_out,sizeof(msg_out),0))<0){
			printf("list_request error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}

	memset(&msg_in,0,sizeof(struct message_s));
	if((len=recv(fd,&msg_in,sizeof(msg_in),0))<0){
			printf("list reply error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
	}

	//network to host byte
	msg_in.length = ntohl(msg_in.length);	
	
	printf("payload byte is: %d\n",msg_in.length-12);
	
	// if payload > INT_MAX, exit(0)
	msg_limit(msg_in.length-12);
	
	ls_buf = malloc(msg_in.length);
	
	// analysis the reply message from server
	check = msgChecker(&msg_in, LIST_REPLY, unused, len);
	errorprint(check);

	if(check==0){	
		len = dataReceiver(fd,msg_in.length-12,ls_buf);
		if(len != (msg_in.length-12) )
        	{
                	printf("recieve not enough bytes\n");
                	return; //error
       		 }

		printf("--------------- file list start ----------------\n");
		printf("%s\n",ls_buf);
		printf("--------------- file list end ------------------\n");
	}
	else
		printf("ERROR: Wrong list reply message\n");
}


void download(char* filename){
		
	int payload = strlen(filename) + 1;
	msg_out = msgGeneration(GET_REQUEST,unused,payload);
	
	// send GET_request to server
	if((len=send(fd, &msg_out,sizeof(msg_out),0))<0){
			printf("get_request error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	// send file name to server 
	if((len=send(fd, filename, strlen(filename), 0))<0){
			printf("Send file name Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	// send \0 to server 
	if((len=send(fd, "\0", 1, 0))<0){
			printf("Send null terminator Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	
	memset(&msg_in,0,sizeof(struct message_s));
	// receive get reply from server
	if((len=recv(fd,&msg_in,sizeof(msg_in),0))<0){
			printf("Receive get reply error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
	}
	
	//network to host byte
	msg_in.length = ntohl(msg_in.length);	
	
	// analysis the reply message server
	check = msgChecker(&msg_in, GET_REPLY, 1, len);
	errorprint(check);

	// request accept & file exist in server
	if(check==0){
		memset(&msg_in,0,sizeof(struct message_s));
		// receive file_data message from server
		if((len=recv(fd,&msg_in,sizeof(msg_in),0))<0){
			printf("Receive get reply error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
		}
		
		//network to host byte
		msg_in.length = ntohl(msg_in.length);	
		
		// if payload  > INT_MAX then  exit
		msg_limit(msg_in.length-12);
		
		
		// analysis the reply message server
		check = msgChecker(&msg_in, FILE_DATA, unused, len);
		errorprint(check);
		if(check ==0){	
	        	
			int dlfd;	
			char path[256] = "filedir/";
			
			len = msg_in.length-12;
			printf("payload bytes is: %d\n",len);
			strcat(path,filename);
			
			if((dlfd=open(path, O_WRONLY | O_TRUNC | O_CREAT,S_IRWXU))>0)
			{
				// dependent on payload to set buf size
				dl_buf = malloc(len+12);
				// receive file_data reply from server
				printf("start to download....\n");
				
				len=dataReceiver(fd,len, dl_buf);
				if(len != (msg_in.length-12) ){
		                	printf("download: data received bytes != payload bytes.\n");
					return;
				}
				len = write(dlfd, dl_buf, len);
				printf("end of download\n");
				close(dlfd);
				if(len ==-1){
					printf("write file error");
				}				
			}
			else
				printf("open file error.\n");
		}
		else
			printf("data reply error\n");
	}
	else 
		printf("download-request rejected.\n");

}


void upload(char* filename){
	int stat_return;
	int payload = strlen(filename) + 1;
	int upfd = open(filename,O_RDONLY,S_IRUSR);
	struct stat upfile;
	if(upfd == -1){
		printf("Error: the file: %s does not exist\n",filename);
		return;
	}
	
	if((stat_return = fstat(upfd, &upfile))==-1){
		printf("stat error: cannot get file information.\n");
		return;
	}

	msg_out = msgGeneration(PUT_REQUEST,unused,payload);
			
	// send PUT_request to server
	if((len=send(fd, &msg_out,sizeof(msg_out),0))<0){
			printf("put_request error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	// send upload file name to server 
	if((len=send(fd, filename, strlen(filename), 0))<0){
			printf("Send upload-file-name Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	// send \0 to server 
	if((len=send(fd, "\0", 1, 0))<0){
			printf("Send null terminator Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	
	memset(&msg_in,0,sizeof(struct message_s));
	
	// receive put reply from server
	if((len=recv(fd,&msg_in,sizeof(msg_in),0))<0){
			printf("Receive put reply error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
	}
	
	// analysis the reply message server
	check = msgChecker(&msg_in, PUT_REPLY, unused, len);
	errorprint(check);
	
	// upload request accept
	if(check==0){
/*	
		msg_out = msgGeneration(FILE_DATA,unused,upfile.st_size);
		if((len=send(fd,&msg_out, upfile.st_size, 0))<0){
			printf("Send data message Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		len = upfile.st_size;
		printf("payload bytes of upload file is: %d\n",len);
		
		// dependent on payload to set buf size
		up_buf = malloc(upfile.st_size+12);
		int count =0;
		while(count<upfile.st_size){	
			len  = read(upfd,(char*)up_buf+count, len);
			count += len;
			if(len==-1)
			{
				printf("read error\n");
			}
		}
		len = upfile.st_size;

		printf("start to upoad....\n");

		len = dataSender(fd,len,up_buf);
	
		printf("end of upload\n");
		if(len != (upfile.st_size) )
			printf("upload: data sent bytes != file bytes.\n");
*/

		msg_out = msgGeneration(FILE_DATA,unused,upfile.st_size);
		 //host to network byte
	        unsigned int payload;
		payload = upfile.st_size;
		payload = htonl(payload);
		
		if((len=send(fd,&msg_out,12,0))<0){
			printf("Send data file Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		int paylen = upfile.st_size;
		up_buf = malloc(upfile.st_size);
		printf("payload bytes of upload file is: %d\n",paylen);
	//=========================
		if((len=data_out(upfd,paylen,up_buf))!=paylen)
                {
                        printf("exact read bytes(%d) != payload %d",len, paylen);
                        exit(0);
                }
		printf("start to upoad....\n");

	        int len=0;
	        int recvlength=0;
        	while(recvlength<paylen){
                if((len=send(fd,up_buf+recvlength,paylen-recvlength,0))<0){
                        printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
                        exit(0);
                }
                //printf("recv...\n");
                	recvlength+=len;
        	}	



	//	if((len=send(fd,up_buf, payload, 0))<0){
	//		printf("Send data file Error: %s (Errno:%d)\n",strerror(errno),errno);
	//		exit(0);
	//	}
		printf("end of upload\n");
		printf("send / upload bytes is: %d\n",len);
		if(len != (paylen) )
			printf("upload: data sent bytes != file bytes.\n");
	}
	else 
		printf("upload-request rejected.\n");

	close(upfd);
}

void quit(){
	
	msg_out = msgGeneration(QUIT_REQUEST,unused,0);
	// send QUIT_request to server
	if((len=send(fd, &msg_out,sizeof(msg_out),0))<0){
			printf("quit_request error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	memset(&msg_in,0,sizeof(struct message_s));
	// receive quit reply from server
	if((len=recv(fd,&msg_in,sizeof(msg_in),0))<0){
			printf("Receive quit reply error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
	}
	check = msgChecker(&msg_in, QUIT_REPLY, unused, len);
	errorprint(check);
	if(check==0)
		exit(0);
	else
		printf("quit error\n");
			
}

void fsm_control(char** token, int argc){

	// for quit the state looping
	int loop = 1;
	if(token[0]==NULL && state == IDLE)
		printf("Usage: open [IP address] [port]\n");
	else if(strcmp(token[0],"exit")==0 && argc==1)
		exit(1);
	else
	while(loop){	
		switch((int)state)
		{
			case(IDLE):

				if(strcmp(token[0],"open")==0 && argc==3)
					state = IS_OPEN;
				else {	
					printf("Usage: open [IP address] [port]\n");
					loop = 0;
					state = IDLE;
				     }
				break;

			case(IS_OPEN):

				open_conn(token);
				loop = 0;			
				break;

			case(IS_AUTH):
				if(strcmp(token[0],"auth")==0 && argc==3)
					auth_conn(token);
				else{
					printf("ERROR: Authentication not started. Please issue an authentication command.\n");
					printf("usage: auth [USER] [PASSWORD]\n");
					loop =0;	
				    }
				break;

			case(IS_MAIN):
				if(strcmp(token[0],"ls")==0 && argc==1)
					list_file();
				else if(strcmp(token[0],"get")==0 && argc==2)
					download(token[1]);
				else if(strcmp(token[0],"put")==0 && argc==2)
					upload(token[1]);
				else if(strcmp(token[0],"quit")==0 && argc==1)
					quit();
				else{
					printf("\n	Main function usage:\n\n");
					printf("ls		// show all files \n");
					printf("put [FILENAME]  // for upload files\n");
					printf("get [FILENAME]  // for download files\n");
				    }
				loop =0;
				break;
		}	
	}	
}

int main(void){
	
	while(1){
		//int i;
		int argc;
		char* token[128];
		char input[255];
		clean(token);
		showPath();
		gets(input);
		argc = getToken(token, input);
		
		fsm_control(token,argc);
		clean(token);
	};
	return 0;
}
