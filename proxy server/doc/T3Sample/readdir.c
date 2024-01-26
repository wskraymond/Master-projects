#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char** readDir(char* path) { 
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

