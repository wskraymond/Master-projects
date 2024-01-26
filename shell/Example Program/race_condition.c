#include <stdio.h> // required by printf()
#include <stdlib.h>	// required by exit(), srand(), rand()
#include <unistd.h>	// required by fork(), read(), write(), lseek()
#include <time.h>	// required by time()
#include <sys/types.h>	// required by open(), lseek()
#include <sys/stat.h>	// required by open()
#include <fcntl.h>	// required by open()
#include <sys/wait.h>	// required by wait()

#define MAX_RAND_SLEEP	2

#define DATA_TYPE	int  // The data type to be stored on file.
#define DATA_SIZE	sizeof(DATA_TYPE)  // The size of the data type.


void print_msg(char *msg, int num, int show_tab)
{
	if(show_tab)
		printf("\t\t");
	printf("%s\t%d\n", msg, num);
}

void random_sleep()
{
	sleep(rand() % MAX_RAND_SLEEP);
}

/********

  Function: do_parent_job(int fd)

  This parent process runs this function.
  Its job is to INCREASE the value in the shared file by 10.

********/

void do_parent_job(int fd)
{
	DATA_TYPE num;

	srand(time(NULL));

	random_sleep();

	lseek(fd, 0, SEEK_SET);
	if(read(fd, &num, DATA_SIZE) != DATA_SIZE)
	{
		perror("read()");
		exit(1);
	}
	print_msg("read", num, 0);

	num = num + 10;
	print_msg("add", num, 0);
	random_sleep();

	lseek(fd, 0, SEEK_SET);
	if(write(fd, &num, DATA_SIZE) != DATA_SIZE)
	{
		perror("write()");
		exit(1);
	}
	print_msg("write", num, 0);
}

void do_child_job(int fd)
{
	DATA_TYPE num;

	// add 100 to avoid having the same seed as the parent.
	srand(time(NULL) + 100);

	random_sleep();

	lseek(fd, 0, SEEK_SET);
	if(read(fd, &num, DATA_SIZE) != DATA_SIZE)
	{
		perror("read()");
		exit(1);
	}
	print_msg("read", num, 1);

	num = num - 10;
	print_msg("minus", num, 1);
	random_sleep();

	lseek(fd, 0, SEEK_SET);
	if(write(fd, &num, DATA_SIZE) != DATA_SIZE)
	{
		perror("write()");
		exit(1);
	}
	print_msg("write", num, 1);
}

int main(void)
{
	int fd;
	DATA_TYPE num;

	fd = open("tmpfile", O_CREAT | O_RDWR | O_TRUNC, 0600);
	if(fd == -1)
	{
		perror("fopen");
		exit(1);
	}

	num = 10;
	if(write(fd, &num, DATA_SIZE) != DATA_SIZE)
	{
		perror("write()");
		exit(1);
	}

	if(fork())
		do_parent_job(fd);
	else
	{
		do_child_job(fd);
		exit(0);	// required!
	}

	wait(NULL);
	lseek(fd, 0, SEEK_SET);
	if(read(fd, &num, DATA_SIZE) != DATA_SIZE)
	{
		perror("read()");
		exit(1);
	}
	printf("Final value in the shared file: %d\n", num);
	close(fd);

	return 0;
}
