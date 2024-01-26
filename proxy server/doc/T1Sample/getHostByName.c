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
        printf("Usage: %s (name)\n",argv[0]);
        exit(1);
    }
    struct hostent * he;
    struct in_addr ** addrList;
    he=gethostbyname(argv[1]);
    if(he==NULL){
        perror("gethostbyname()");
        exit(2);
    }
    printf("Host Name:%s\n",he->h_name);
    printf("IP Address(es):\n");
    addrList=(struct in_addr**)he->h_addr_list;
    int i;
    for(i=0;addrList[i]!=NULL;i++){
        printf(" %s\n",inet_ntoa(*addrList[i]));
    }
    return 0;
}
