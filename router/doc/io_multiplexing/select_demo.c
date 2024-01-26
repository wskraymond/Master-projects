/*
	A demonstration program of select system call
	---------------------------------------------

	Purpose: - a demonstration program for the course CSC4430.
		 - the program can be set to be blocking, using select().
		 - the program can be set to wait for input with a periodic timeout interval,
		   using select() too.

	Note on code display
	--------------------

	- Not designed for terminal with 80 columns.
	  (The implementation environment is a terminal with 110 columns.)
	- Tab size is of 8 spaces.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>		// required by select()
#include <sys/select.h>		// required by select()
#include <sys/types.h>		// required by select()
#include <sys/times.h>		// required by struct timeval and select()
#include <string.h>		// required by strlen()

#define BLOCK_MODE	0	// to indicate blocking mode
#define STDIN_FD	0	// file descriptor of STDIN
#define	BUF_SIZE	1024	// size of the input buffer

int main(int argc, char **argv)
{
	fd_set rfds;		// required by select()
	struct timeval tv;	// required by select()
	int retval;		// return value of select()
	int mode;		// blocking or non-blocking mode chosen by user
	int sec;		// timeout input from user
	char buf[BUF_SIZE];	// buffer for input

	if(argc < 2 || (argc == 2 && atoi(argv[1]) == 1))
	{
		fprintf(stderr, "Usage: %s [0|1] (seconds)\n\n", argv[0]);
		fprintf(stderr, "  1st argument\n");
		fprintf(stderr, "  ------------\n");
		fprintf(stderr, "    0 for blocking mode\n");
		fprintf(stderr, "    1 for timeout mode\n\n");
		fprintf(stderr, "  2nd argument\n");
		fprintf(stderr, "  ------------\n");
		fprintf(stderr, "    States the number of seconds (timeout mode only)\n\n");

		exit(1);
	}

	mode = atoi(argv[1]);
	if(mode != BLOCK_MODE)
		sec = atoi(argv[2]);


////  Main Loop  ////

	while(1)
	{
		FD_ZERO(&rfds);			// initial rfds, a fd_set structure
		FD_SET(STDIN_FD, &rfds);	// Set to monitor STDIN

		if(mode != BLOCK_MODE)
		{
			tv.tv_sec = sec;	// the timeout period is defined by the user
			tv.tv_usec = 0;
		}

		if(mode == BLOCK_MODE)
			retval = select(1, &rfds, NULL, NULL, NULL);
		else
			retval = select(1, &rfds, NULL, NULL, &tv);

		printf("** return value is %d **\n", retval);

		if(retval == -1)
			perror("select()");				// no break here
		else if(retval == 0)
			printf("No data within %d sec.\n", sec);	// report timeout is reached
		else
			break;						// break the while-loop
	}

////  Print result of select  ////

	if(FD_ISSET(0, &rfds))
		printf("\"FD_ISSET(0, &rfds)\" returns true\n");
	else
		printf("\"FD_ISSET(0, &rfds)\" returns false\n");

////  Print STDIN input  ////

	printf("\nThe input string is:\n");
	while(fgets(buf, BUF_SIZE, stdin) != NULL)	// Do you know why I use a while-loop here?
	{
		printf("%s", buf);
		if(buf[strlen(buf)-1] == '\n')		// Hint: fgets() works closely with '\n'
			break;
	}

	return 0;
}
