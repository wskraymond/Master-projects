#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>	// "struct sockaddr_in"
#include <arpa/inet.h>	// "in_addr_t"
#include "common.h"


void main_task(in_addr_t ip, unsigned short port)
{
	int buf;
	int fd;
	struct sockaddr_in addr;   //server'address = ip + port (above)
	unsigned int addrlen = sizeof(struct sockaddr_in); 

	fd = socket(AF_INET, SOCK_STREAM, 0);

	if(fd == -1)
	{
		perror("socket()");
		exit(1);
	}

	memset(&addr, sizeof(struct sockaddr_in), 0);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip;    //no need to convert // already in NB order
	addr.sin_port = htons(port);

	if( connect(fd, (struct sockaddr *) &addr, addrlen) == -1 )
	{
		perror("connect()");
		exit(1);
	}
	struct message_s msg = msgGeneration(DV,-1,-1,-1,0);
	msgSender(fd,&msg);
	printf("press key\n");
	getchar();
	printf("DONE\n");
	close(fd);
}

int main(int argc, char **argv)
{
	in_addr_t ip;
	unsigned short port;

	if(argc != 3)
	{
		fprintf(stderr, "Usage: %s [IP address] [port]\n", argv[0]);
		exit(1);
	}


	if( (ip = inet_addr(argv[1])) == -1 )  // convert char* into in_addr_t in NB order
	{
		perror("inet_addr()");
		exit(1);
	}

	port = atoi(argv[2]);

	main_task(ip, port);
	return 0;
}
