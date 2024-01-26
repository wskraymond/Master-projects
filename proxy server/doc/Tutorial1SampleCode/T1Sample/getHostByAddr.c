# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <netdb.h>
# include <arpa/inet.h>

int main(int argc, char** argv){
    if(argc!=2){
        printf("Usage: %s (IP Address)\n",argv[0]);
        exit(1);
    }

    struct hostent *he;
    struct in_addr addr;

    inet_pton(AF_INET, argv[1], &addr);
    he = gethostbyaddr(&addr, sizeof(addr), AF_INET);
    printf("Host name: %s\n", he->h_name);
    return 0;
}
