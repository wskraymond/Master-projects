#ifndef __MYFTP__

#define __MYFTP__
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

struct message_s {
	char protocol[6];	/* protocol magic number (6 bytes) */
	char type;	/* type (1 byte) */
	char status;	/* status (1 byte) */
	int length;	/* length (header + payload) (4 bytes) */
} __attribute__ ((packed));

#define unused 'U'	//only for label , but no need to check
#define	OPEN_CONN_REQUEST 0xA1
#define OPEN_CONN_REPLY 0xA2
#define	AUTH_REQUEST	0xA3
#define	AUTH_REPLY	0xA4
#define	LIST_REQUEST	0xA5
#define	LIST_REPLY	0xA6
#define	GET_REQUEST	0xA7
#define	GET_REPLY	0xA8
#define	FILE_DATA	0xFF
#define	PUT_REQUEST	0xA9
#define	PUT_REPLY	0xAA
#define	QUIT_REQUEST	0xAB
#define	QUIT_REPLY	0xAC

//size_payload => for filename: strlen(payload) + 1 in parameter list.

char magicNum[6] = {0xe3,'m','y','f','t','p'};
struct message_s msgGeneration(char type,char status,int size_payload)
{
	struct message_s msg;
	memset(&msg,0,sizeof(msg));
	msg.protocol[0] = 0xe3;
	msg.protocol[1] = 'm';
	msg.protocol[2] = 'y';
	msg.protocol[3] = 'f';	
	msg.protocol[4] = 't';
	msg.protocol[5] = 'p';
	msg.type = type;
	msg.status = status;
	msg.length = 12 + size_payload;
	//host to network byte
	msg.length = htonl(msg.length);
	return msg;
}

//pass &msg(address) to be check
//all in parameter list are about expected type of message
//len are the lenght returned by recv()!!!!
// return 0 -> all are expected one
//return 1 -> not myftp protocol message
//return 2 -> not expected type
//return 3 -> not expected status
int msgChecker(struct message_s* item,unsigned char type,char status,int len)
{
	if( (memcmp((*item).protocol,magicNum,6)==0) && (len==12) )
	{
		if( ((unsigned char)(*item).type) == type)
		{
			if(status!=unused)
			{
				if( (*item).status == status )
					return 0;
				else
					return 3;
			}
			else
				return 0;
		}
		else
			return 2; 
	}
	else
		return 1;
}

void errorprint(int error)
{
	switch(error)
	{
		case 1:
			printf("Non-MYFTP protocol received!\n");
			break;
		case 2:
			printf("Unepected MYFTP message received\n");
			break;
		case 3:
			printf("status error\n");
	}
}


char** readDir(char* path) 
{ 
    char** returnBuffer=(char**)calloc(512,sizeof(char*));
    int return_code; 
    DIR *dir; 
    struct dirent *entry = (struct dirent*) calloc (sizeof(struct dirent)+256,1); 
    struct dirent *result ;

    if ((dir = opendir(path)) == NULL) 
        perror("opendir() error");
    else { 
        int index=0;
        //puts("contents of directory:"); 
        while(1){ 
            return_code = readdir_r(dir, entry, &result); 
#ifdef Linux
            if(result == NULL ) 
#else
            if(return_code==0) 
#endif
                break; 
            //printf("  %s\n", entry->d_name); 
            returnBuffer[index] = (char*) calloc(256,sizeof(char));
            memcpy(returnBuffer[index],entry->d_name,strlen(entry->d_name));
            index++;
        } 
        closedir(dir); 
    }
    return returnBuffer;
}
#endif

