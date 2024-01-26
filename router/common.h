#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

//agent's
#define	DV 0xA1
#define UPDATE 0xA2
#define	SHOW	0xA3
#define	RESET	0xA4
#define	RESPONSE 0xA5
//router's 
#define	DVMSG 0xA6
#define	CLOSE 0xA7

char magicNum[3] = {'R','I','P'};
struct message_s 
{
	char protocol[3];	/* protocol magic number (6 bytes) */
	char type;	/* type (1 byte) */
	int source; //source RID or the RID of the target agent send
	int dest;	// dest RID
	int weight; //weight or the number of DVs that router received
	int payload;	/* payload + 1 for '\0'*/
} __attribute__ ((packed));

//Protocol
/*
1.dv message
	struct message_s msg = msgGeneration(DV,-1,-1,-1,0);
2.update: 1,2,4 message
	struct message_s msg = msgGeneration(UPDATE,1,2,4,0);
3.show: 4
	send to router 4
	struct message_s msg = msgGeneration(SHOW,-1,-1,-1,0);
4.reset: 4
	send to router 4
	struct message_s msg = msgGeneration(RESET,-1,-1,-1,0);
5. received response 
For receive a response from router, use below one to check
if( (memcmp((*msg_in).protocol,magicNum,3)==0) && (len==sizeof(struct message_s)) )	

//magicNum already in common.h

received msg is from *msg_out = msgGeneration(RESPONSE,router's RID,-1,number of DV packet recieved,payload for routing table);

Followed data(routing table) is like that:
// dest nextHop cost

2 3 5\n
4 3 4\n
6 3 8\n\0   //including '\0'

*/

struct message_s msgGeneration(char type,int source,int dest,int weight,int size_payload)
{
	struct message_s msg;
	memset(&msg,0,sizeof(msg));
	msg.protocol[0] = 'R';
	msg.protocol[1] = 'I';
	msg.protocol[2] = 'P';
	msg.type = type;
	msg.source	= htonl(source);
	msg.dest	= htonl(dest);
	msg.weight	= htonl(weight);
	msg.payload = htonl(size_payload);
	return msg;
}


int msgReceiver(int client_sd,struct message_s* msg)
{
	int len;
	memset(msg,0,sizeof(struct message_s));
	if((len=recv(client_sd,msg,sizeof(struct message_s),0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	//network to host byte
	msg->source	= ntohl(msg->source);
	msg->dest	= ntohl(msg->dest);
	msg->weight	= ntohl(msg->weight);
	msg->payload = ntohl(msg->payload);
	printf("bytes received: %d\n",len);
	return len;
}

int msgSender(int client_sd,struct message_s* msg_out)
{
	int len;
	if((len=send(client_sd,msg_out,sizeof(struct message_s),0))<0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	printf("bytes sent: %d\n",len);
	return len;
}

int dataReceiver(int client_sd,int size,void* buf)
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
	printf("bytes received: %d\n",recvlength);
	return recvlength;
}

int dataSender(int client_sd,int size,void* buf)
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
	printf("bytes sent: %d\n",sentlength);	
	return sentlength;
}
