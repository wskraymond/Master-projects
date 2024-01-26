/*
	A chatroom program implemented using named pipe
	-----------------------------------------------

	Purpose: a demonstration program for the course CSC4430.

	Note on code display
	--------------------

	- Not designed for terminal with 80 columns.
	  (The implementation environment is a terminal with 110 columns.)
	- Tab size is of 8 spaces.

	Design
	------

	- This chatroom program is designed for two people to run the same program.

	- Instead of using sockets, this implementation uses two named pipes.

	- The two named pipes must be created before this program is executed.

	- The two named pipes must be inputted to the program by command-line arguments.

	- One named pipe is for reading messages from another program instance;
	  another named pipe is for sending messages to another program instance;

	- Using select() to perform I/O multiplexing on the two input streams:
	  STDIN and input named pipe.

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>		// required by strcmp()
#include <sys/types.h>		// required by stat(), select()
#include <sys/stat.h>		// required by stat()
#include <unistd.h>		// required by stat(), select()
#include <sys/select.h>		// required by select()
#include <sys/time.h>		// required by struct timeval

#define STDIN_FD	0	// the file descriptor number of STDIN
#define BUF_SIZE	1024	// the buffer size

#define MAX(a,b)	((a) > (b) ? (a) : (b))		// required to compare fd numbers.

int main(int argc, char **argv)
{
	struct stat file_stat;	// used by stat()
	int pipe_fd;		// the file descriptor number of the input pipe
	int max_fd;		// the max fd number monitored by select()
	int i;			// need by for-loop
	fd_set read_fds;	// the fd_set for reading, required by select()
	FILE *pipe_ptr[2];	// the FILE pointers for the input and the output pipes
	char buf[BUF_SIZE];	// the buffer that stores input and output messages
	int INPUT_PIPE;		// a constant to denote an input pipe (not declared as const)
	int OUTPUT_PIPE;	// a constant to denote an output pipe (not declared as const)

//// Check the number of arguments ////

	if( argc != 4 || (strcmp(argv[3], "yes") && strcmp(argv[3], "no")) )
	{
		fprintf(stderr, "Usage: %s [pipe 1] [pipe 2] [first client? (yes|no)]\n", argv[0]);
		exit(1);
	}

	if(strcmp(argv[3], "yes") == 0)
	{
		INPUT_PIPE = 0;
		OUTPUT_PIPE = 1;
	}
	else
	{
		INPUT_PIPE = 1;
		OUTPUT_PIPE = 0;
	}

//// There are two named pipes. So ... ////

	for(i = 0; i < 2; i++)
	{
	//// Check whether the file exists or not ////

		if(stat(argv[i+1], &file_stat) == -1)
		{
			perror("stat()");
			exit(1);
		}

	//// Check whether the file is a pipe or not ////

		if(!S_ISFIFO(file_stat.st_mode))
		{
			fprintf(stderr, "Error: %s is not a pipe. Exiting.\n", argv[i+1]);
			exit(1);
		}

		if(i == INPUT_PIPE)
		{
		//// Check whether the pipe can be opened for reading or not ////

			printf("Waiting for the input pipe connection \"%s\" ... ", argv[i+1]);
			fflush(stdout);
			if((pipe_ptr[i] = fopen(argv[i+1], "r")) == NULL)
			{
				perror("fopen()");
				exit(1);
			}
			printf("done.\n");
		}
		else
		{
		//// Check whether the pipe can be opened for writing or not ////

			printf("Waiting for the output pipe connection \"%s\" ... ", argv[i+1]);
			fflush(stdout);
			if((pipe_ptr[i] = fopen(argv[i+1], "w")) == NULL)
			{
				perror("fopen()");
				exit(1);
			}
			printf("done.\n");

		} // end if

	} // end for

	printf("[Program is ready.]\n\n");

	pipe_fd = fileno(pipe_ptr[INPUT_PIPE]);		// for select()
	FD_ZERO(&read_fds);				// for select()
	max_fd = MAX(pipe_fd, STDIN_FD);		// for select()

//// The main loop ////

	while(1)
	{
		FD_SET(pipe_fd, &read_fds);		// for select()
		FD_SET(STDIN_FD, &read_fds);		// for select()

		printf("$ ");				// for prompt
		fflush(stdout);

	//// Invoke select(), without timeout ////

		if(select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1)	// return value is useless.
		{
			perror("select()");
			exit(-1);

		} // end if

	//// If INPUT PIPE has data available ... ////

		if(FD_ISSET(pipe_fd, &read_fds))
		{
			if(fgets(buf, BUF_SIZE, pipe_ptr[INPUT_PIPE]) == NULL)
			{
			//// Enter here when EOF is detected at the INPUT PIPE ////

				printf("[Terminated by friend. Goodbye.]\n");
				fclose(pipe_ptr[INPUT_PIPE]);
				fclose(pipe_ptr[OUTPUT_PIPE]);

				break;		// break the while-loop

			} // end if

			printf("[From friend]$ %s", buf);
			fflush(stdout);

		} // end if. Don't use else here!

	//// If STDIN has data available ... ////

		if(FD_ISSET(STDIN_FD, &read_fds))
		{
			if(fgets(buf, BUF_SIZE, stdin) == NULL)
			{
			//// Enter here when EOF is detected at the STDIN ////

				printf("[Terminated by me. Goodbye.]\n");
				fclose(pipe_ptr[INPUT_PIPE]);
				fclose(pipe_ptr[OUTPUT_PIPE]);

				break;		// break the while-loop

			} // end if

			fprintf(pipe_ptr[OUTPUT_PIPE], "%s", buf);
			fflush(pipe_ptr[OUTPUT_PIPE]);

		} // end if

	} // end while

	return 0;

} // end main
