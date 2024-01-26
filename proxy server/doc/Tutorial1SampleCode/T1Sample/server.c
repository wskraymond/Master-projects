# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>

# define PORT 12345

int main(int argc, char** argv){
	int sd=socket(AF_INET,SOCK_STREAM,0);
	int client_sd;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	server_addr.sin_port=htons(PORT);
	if(bind(sd,(struct sockaddr *) &server_addr,sizeof(server_addr))<0){
		printf("bind error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	if(listen(sd,3)<0){
		printf("listen error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	int addr_len=sizeof(client_addr);
	/*if((client_sd=accept(sd,(struct sockaddr *) &client_addr,&addr_len))<0){
		printf("accept erro: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}*/
    printf("BEFORE ACCEPT\n");
		if((client_sd=accept(sd,(struct sockaddr *) &client_addr,&addr_len))<0){
			printf("accept erro: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
    printf("AFTER ACCEPT\n");
	while(1){
		char buff[100];
		int len;
		if((len=recv(client_sd,buff,sizeof(buff),0))<0){
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
		}
		buff[len]='\0';
		printf("RECEIVED INFO: ");
		if(strlen(buff)!=0)printf("%s\n",buff);
		if(strcmp("exit",buff)==0){
			close(client_sd);
			break;
		}
	}
	close(sd);
	return 0;
}
