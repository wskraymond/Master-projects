#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char** readDir(char*);

int main(){
    char** c=readDir("test/");
    int index=0;
    while(c[index]!=NULL){
        printf("%s\n",c[index]);
        index++;
    }
    return 0;
}

