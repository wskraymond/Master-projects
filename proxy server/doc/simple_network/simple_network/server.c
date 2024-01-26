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


void main_loop(unsigned short port)
{
	int fd, accept_fd, count, buf;
	struct sockaddr_in addr, tmp_addr;
	unsigned int addrlen = sizeof(struct sockaddr_in);  // must initialize it first!!!!

	fd = socket(AF_INET, SOCK_STREAM, 0);

	if(fd == -1)
	{
		perror("socket()");
		exit(1);
	}

	memset(&addr, sizeof(struct sockaddr_in), 0);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if(bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
	{
		perror("bind()");
		exit(1);
	}

	if( listen(fd, 1024) == -1 )
	{
		perror("listen()");
		exit(1);
	}

	while(1)
	{
		// tmp_addr, addrlen are return value about the info of client.
		if( (accept_fd = accept(fd, (struct sockaddr *) &tmp_addr, &addrlen)) == -1)
		{
			perror("accept()");
			exit(1);
		}

		count = read(accept_fd, &buf, sizeof(buf));   // caution: buf read is in NB order

		if(count == -1)
		{
			perror("reading...");
			exit(1);
		}

		if(count == 0)
		{
			fprintf(stderr, "bye\n");
			exit(1);
		}

		close(accept_fd);

		printf("result = %x\n", buf);
	}
}

int main(int argc, char **argv)
{
	unsigned short port;

	if(argc != 2)
	{
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	port = atoi(argv[1]);

	main_loop(port);

	return 0;
}
