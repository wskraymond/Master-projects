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
    char addr[100];
    gethostname(addr,sizeof(addr));
    printf("Host name: %s\n", addr);
    return 0;
}

