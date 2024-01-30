#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h> // for process creation
#include<sys/wait.h> // for process creation
#include<errno.h>    // for usage of varible errno
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>

char cmdLine[40];
char currentPath[80];
char* tokens[10];
char types[10][40];
int builtInDetect = 0;
int error = 0;

//__________job control_________________
char cmdTmp[80];
char cmdJob[10][80];
int jobnum = 0;
int jobpid[10][10];

//_____________________________________

void tokenizer()
{
	int i=0;
	strcpy(cmdTmp,cmdLine);
	tokens[i]=strtok(cmdLine," ");
	while(tokens[i]!=NULL){
		i++;
		tokens[i]=strtok(NULL," ");
	}
	i=0;
}
int checkIllegal(char* aToken){
	char illegal[] = "><|*!'\"";
	int i = strcspn(aToken,illegal);
	if(i<strlen(aToken))
		return 0;   //legal
	else
		return 1;
}

void outPut()
{
		if(tokens[0]!=NULL)
		if(builtInDetect && error)
			printf("%s: wrong number of arguments\n",tokens[0]);
		else if(error)
                	printf("Error: invalid input command line\n");	
}

enum STATES {WAIT_CMD, WAIT_ARG, WAIT_IN_FILE, WAIT_OUT_FILE, WAIT_MORE, WAIT_ONE_ARG, NO_MORE};
enum STATES state = WAIT_CMD;
int  checkSyntax( )
{
        int i=0;
    	error=0;
        int recursive = 0;
	builtInDetect = 0;
	int fileQuota = 0;
        state = WAIT_CMD;
        while( tokens[i]!=NULL)
        {	
		switch((int)state){
			 case(WAIT_CMD):
                                if(checkIllegal(tokens[i])==0)
                                        error = 1;
                                else if(strcmp(tokens[i],"cd")==0 || strcmp(tokens[i],"exit")==0 || strcmp(tokens[i],"fg")==0 || strcmp(tokens[i],"jobs")==0 )
                                {
                                        if(strcmp(tokens[i],"cd")==0 || strcmp(tokens[i],"fg")==0 )
                                                state = WAIT_ONE_ARG;
                                        else
                                                state = NO_MORE;
					builtInDetect = 1;
                                        strcpy(types[i], "Built-in Command");
                                }
                                else
                                {
                                        state = WAIT_ARG;
                                        strcpy(types[i],"Command Name");
                                }
                                break;
			 case(WAIT_ARG):
                                if(strcmp(tokens[i],"|")==0)
                                {
                                        state = WAIT_CMD;
                                        strcpy(types[i],"Pipe");
                                        recursive++;
                                }
                                else if(strcmp(tokens[i],"<")==0)
                                {
                                        state = WAIT_IN_FILE;
                                        strcpy(types[i],"Redirect Input");
                                }
                                else if(strcmp(tokens[i],">")==0 || strcmp(tokens[i],">>")==0 )
                                {
                                        state = WAIT_OUT_FILE;
                                        strcpy(types[i],"Redirect Output");
                                }
                                else if(checkIllegal(tokens[i])==0)
                                        error = 1;
                                else
					strcpy(types[i],"Argument");
                                break;
			case(WAIT_ONE_ARG):
                                if(checkIllegal(tokens[i])==0)        
                                        error = 1;
                                else
                                {
                                        state = NO_MORE;
                                        strcpy(types[i],"Argument");
                                }
                                break;
			case(WAIT_IN_FILE):
                                if(recursive==0)
                                        if(checkIllegal(tokens[i])==0)
                                                 error = 1;
                                        else
                                        {
                                                state = WAIT_MORE;
                                                strcpy(types[i],"Input Filename");
						fileQuota++;
                                        }
				else
                               		 error = 1;
                                break;
                        case(WAIT_OUT_FILE):
                                        if(checkIllegal(tokens[i])==0)
                                                error = 1;
                                        else
                                        {
                                                state = WAIT_MORE;
                                                strcpy(types[i],"Output Filename");
						fileQuota++;
                                        }
                                    break;
			 case(WAIT_MORE):
                                if(strcmp(tokens[i],"<")==0 && strcmp(types[i-1],"Output Filename")==0 && fileQuota<=2)
                                {
                                        state = WAIT_IN_FILE;
                                        strcpy(types[i],"Redirect Input");
                                }
                                else if( (strcmp(tokens[i],">")==0 || strcmp(tokens[i],">>")==0) &&  strcmp(types[i-1],"Input Filename")==0  && fileQuota<=2)
                                {
                                        state = WAIT_OUT_FILE;
                                        strcpy(types[i],"Redirect Output");
                                }
				else if(recursive==0)
				{
                                        if(strcmp(tokens[i],"|")==0 && strcmp(types[i-1],"Input Filename")==0) // for <cmd><inpput redirect> | only
                                        {
                                                state = WAIT_CMD;
                                                strcpy(types[i],"Pipe");
                                                recursive++;
                                        }
					else
						error = 1;
				}
				else
					error = 1;
				
                            break;
                        case(NO_MORE):
                                error = 1;   // for the case that cd <path> <arg> , a redundent arg.
                        break;
		}
		i++;
	}
	if(state==WAIT_IN_FILE || state==WAIT_OUT_FILE || state == WAIT_CMD || recursive>2 || fileQuota>=3)
        	error = 1;
	
	return error;
}

void builtInCMD()
{
       	int i =0;
	int index = 0;
	if(strcmp(tokens[0],"cd")==0)
                chdir(tokens[1]);
	else if(strcmp(tokens[0],"fg")==0)
	{
		if(jobnum!=0)
		{
		int jobfg = atoi(tokens[1]);
		int status;
		i=0;
		printf("fg start\n");
		while(jobpid[jobfg-1][i]!=-1 && i<10)
		{
			printf("num of processes for job %d\n", jobfg);
			if(kill(jobpid[jobfg-1][i], SIGCONT) == -1)
			{
				printf("The %dth job cannot become foreground\n", jobfg);
			}
			else
			{
				printf("wait\n");
				if(waitpid(jobpid[jobfg-1][i],&status,WUNTRACED)!=jobpid[jobfg-1][i])
					printf("Error:A job startbut but in parrallel with the shelll\n");
			}
			i++;
		}
		
		//tmp job
		char tmp[40];
		strcpy(tmp,cmdJob[jobfg-1]);
		int pidtmp[10];
		for(i=0;i<10;i++)
			if(jobpid[jobfg-1][i]!=-1)
				pidtmp[i]=jobpid[jobfg-1][i];
			else
				pidtmp[i] = -1;
		
		if(WIFSTOPPED(status))
		{
			for(index=jobfg;index<jobnum;index++)  //index is the term after the fg term
			{
				strcpy(cmdJob[index-1],cmdJob[index]);
				for(i=0;i<10;i++)
                        		if(jobpid[index][i]!=-1)
                                		jobpid[index-1][i]=jobpid[index][i];
                        		else
                                		jobpid[index-1][i]= -1;
			}
			strcpy(cmdJob[jobnum-1],tmp);
			for(i=0;i<10;i++)
                        	if(pidtmp[i]!=-1)
                                	jobpid[jobnum-1][i]=pidtmp[i];
                        	else
                                	jobpid[jobnum-1][i] = -1;
		}
		else   //if 
		{
                        for(index=jobfg;index<jobnum;index++)  //index is the term after th
                        {
                                strcpy(cmdJob[index-1],cmdJob[index]);
                                for(i=0;i<10;i++)
                                        if(jobpid[index][i]!=-1)
                                                jobpid[index-1][i]=jobpid[index][i];
                                        else
                                                jobpid[index-1][i]= -1;
                        }
			  for(i=0;i<10;i++)
		                jobpid[jobnum-1][i] = -1;
			jobnum--;       

		}
		}
		else 
			printf("no jobs to be fg\n");
		
	}
	else if(strcmp(tokens[0],"jobs")==0)
	{
		for(i=0;i<jobnum;i++)
		{
			printf("%d %s\n",(i+1), cmdJob[i]);
		}
	}
        else if(strcmp(tokens[0],"exit")==0)
	{
		if(jobnum==0)
                	exit(1);
		else
			printf("It cannot exit because there are still jobs here\n");
	}  
}

int cmdInterpretor()  // return error
{
	int correct;
	tokenizer();
        correct = checkSyntax();
        outPut();
	return correct;
}

int processCreation(char** args, char** file, int outMode, int* front_fd, int* back_fd, int** fds, int noCMD, int ioPort)  //when this func is called, that means the command meets the sy
{	
	int pid;
	int error = 0;
	if((pid=fork())==0)
	{
		signal(SIGINT,SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		signal(SIGSTOP, SIG_DFL);
		signal(SIGKILL, SIG_DFL);
		//child process
		//for Input Redirection
			if(file[0] != NULL)
			{
				int in_fd = open(file[0], O_RDONLY);
				if(in_fd == -1)
				{
					printf("[%s]: no such file or directory\n",file[0]);
					exit(-1);
				}
				if(dup2(in_fd,0) == -1)
				{
					printf("[%s]: Permission denied\n", file[0]);  //??
					exit(-1);
				}
				if(close(in_fd) == -1)
				{
					printf("[%s]: The File cannot be closed\n", file[0]);  //??
					exit(-1);
				}
			}
			
		//for Output Redirection
			if(file[1] != NULL)
			{	
				int out_fd;
				if(outMode)
					out_fd = open(file[1], O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
				else
					out_fd = open(file[1], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
				if(out_fd == -1){
					printf("[%s]: no such file or directory\n", file[1]);
					exit(-1);
				}
				if(dup2(out_fd, 1)==-1){
					printf("[%s]:Permission denied\n", file[1]);
					exit(-1);
				}
				if(close(out_fd)==-1){
					printf("[%s]: The File cannot be closed\n", file[1]);  //??
					exit(-1);
				}
			}

		error = 0;
		//pipe           //ioPort = o => do nothing, 1 => first process, 2=> middle , 3=> last)
		switch(ioPort)
		{
			case 1:
				if(dup2(front_fd[1],1)==-1){
					printf("Permission denied\n");
					error = 1;
				}
				break;
			case 2:
				if(dup2(front_fd[0],0)==-1 ){
					printf("Permission denied\n");
					error = 1;
				}
				if( dup2(back_fd[1],1)==-1)
				{
					printf("Permission denied\n");
					error = 1;
				}
				break;
			case 3:
				if(dup2(back_fd[0],0)==-1 ){
					printf("Permission denied\n");
					error = 1;
				}
				break;			
		}
		
		int i = 0;
		for(i=0;i<noCMD;i++)	//close all fds in the child process before program change or process exits
                {
                        if( close(fds[i][0])==-1 || close(fds[i][1])==-1 )
			{
				 printf("The file descriptors cannot be closed\n");
                                 error = 1;
			}
                }

		if(error)
		{
			printf("Error occur\n");
			exit(-1);
		}

		setenv("PATH","/bin:/bin/usr:.",1); 
		if(execvp(args[0],args))
		{  // return value?
			if(errno == ENOENT)
				printf("[%s]: command not found\n",args[0]);
			else
				printf("[%s]: unknown error\n", args[0]);
			exit(-1);
		}
	}

	return pid;     //???
}

int  cmdExe()
{
	int i=0;
	int j=0;
	char*** command = (char***)malloc(sizeof(char**)*10);
	char*** files = (char***)malloc(sizeof(char**)*10);
	for(i=0;i<10;i++)
	{
		command[i] = malloc(sizeof(char*)*10);
		files[i] = malloc(sizeof(char*)*2);
		files[i][0] = NULL;
		files[i][1] = NULL;
		for(j=0;j<10;j++)
			command[i][j] = NULL;
	}                                                  // initailize with NULL                  !!!!!!!!!!!!!!!!
	int noCMD = 0;           // 0 means at least one command.
	int k = 0;
	
	
	//seperate command and save files into process
	i =0;
	int outMode = 0;
	command[0] = malloc(sizeof(char*)*10); 
	while(tokens[i]!=NULL) 
	{
		if( strcmp(types[i],"Command Name")==0 || strcmp(types[i],"Argument")==0 )
		{
				command[noCMD][k] = tokens[i];
				k++;
		}
		else if(strcmp(types[i],"Pipe")==0)
		{
			command[noCMD][k] = NULL;
			noCMD++;
			k = 0; 
		}
		else if(strcmp(types[i],"Input Filename")==0)
		{
			files[noCMD][0] = tokens[i];
		}
		else if(strcmp(types[i],"Output Filename")==0)
		{
			if(strcmp(tokens[i-1],">")==0)
                                outMode = 0;
                        else if(strcmp(tokens[i-1],">>")==0)
                                outMode =1;

			files[noCMD][1] = tokens[i];
		}

		if(tokens[i+1]==NULL)
			command[noCMD][k+1] = NULL;
		i++;		
	}
	

	//Execution of the command
	int** fds = malloc(sizeof(int*)*noCMD);
	for(i=0;i<noCMD;i++)
		fds[i] = malloc(sizeof(int)*2);
	int pid[noCMD+1];
	int error = 0;
	int failPipe = 0;
	if(noCMD>0)
	{
		for(i=0;i<noCMD;i++)
			if(pipe(fds[i])==-1)
			{
				failPipe = 1;
				error = 1;
				printf("cannot pipe");
            }
		
		
		if(!failPipe)
		{
			for(i=0; i<(noCMD+1); i++)
			{
				if(i==0)
				{
					pid[0] = processCreation(command[0],files[0],outMode,fds[0],NULL,fds,noCMD,1);
                	if(pid[0]==-1)
					{
                        			printf("The command 0 cannot be executed");
						error = 1;
						break;
					}
				}	
				else if(i==noCMD)
				{
			 		pid[noCMD] = processCreation(command[noCMD],files[noCMD],outMode,NULL,fds[noCMD-1],fds,noCMD,3);
                 	if(pid[noCMD]==-1)
					{
                        			printf("The command %d cannot be executed",noCMD);
						error = 1;
						break;
					}
				}
				else
				{
					pid[i]=processCreation(command[i],files[i],outMode,fds[i-1],fds[i],fds,noCMD,2);
					if(pid[i]==-1)
					{
                        			printf("The command %d cannot be executed",i);
						error = 1;
						break;
					}
				}
			}
		}

		for(i=0;i<noCMD;i++)  //condition for cannot pipe ????????????????
		{	
			close(fds[i][0]); 
			close(fds[i][1]);
		}
	}
	else 
	{
		pid[0] = processCreation(command[0],files[0],outMode,NULL,NULL,NULL,noCMD,0);
		if(pid[0]==-1)
			error = 1;
	}
	
	int status;	
	//wait for child.
	for(i=0; i<(noCMD+1)&& pid[i]!=-1 ;i++)
	{
		waitpid(pid[i],&status,WUNTRACED);
		if(WIFSTOPPED(status))
		{
			jobpid[jobnum][i] = pid[i];
		}	
		
	}
	if(WIFSTOPPED(status))
	{	
		strcpy(cmdJob[jobnum],cmdTmp);
		jobnum++;
	}

	// free dynamic memory
	for(i=0; i<10 ; i++)
	{
		free(command[i]);
		free(files[i]);
	}
	for(i=0;i<noCMD;i++)
		free(fds[i]);
	free(fds);
	free(command);
	free(files);
	return error;
}
int main(void)
{
	int a ,b;
	for(a=0; a<10;a++)
        	for(b=0;b<10;b++)
                	jobpid[a][b] = -1;
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGSTOP, SIG_DFL);
	signal(SIGKILL, SIG_DFL);
	while(1)
	{
		builtInDetect = 0;
                getcwd(currentPath,80);
                printf("[3150 shell:%s]$ ",currentPath);
		gets(cmdLine);
		if(!cmdInterpretor() && !builtInDetect &&  (error==0))
			if(cmdExe())
				printf("The Command Line cannot be executed.\n");
		if(builtInDetect && (error==0) )
			builtInCMD();
	}
	return 0;
	
}






