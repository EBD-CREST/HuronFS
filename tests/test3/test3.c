#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <sys/time.h>

//access to a remote file using MPI
//test function:
//open, read, write, flush, close

const int ROOT=0;

#define TIME(st, et) (((et).tv_sec-(st).tv_sec)*1000000+(et).tv_usec-(st).tv_usec)

int main(int argc, char ** argv)
{
	int fd;
	volatile int j=1;
	int rank, number, i=0;
	char *buffer=NULL;
	size_t total_size=0;
	ssize_t ret;
	struct stat file_stat;
	struct timeval st, et;
	fd=open("../../../test1", O_RDONLY);
	fstat(fd, &file_stat);
	if(NULL == (buffer=malloc(file_stat.st_size)))
	{
		perror("malloc");
		return EXIT_FAILURE;
	}

	gettimeofday(&st, NULL);

	if(-1 == (ret=read(fd, buffer, file_stat.st_size)))
	{
		perror("read");
		return EXIT_FAILURE;
	}
	total_size+=ret;

	gettimeofday(&et, NULL);

	printf("total size %lu\n", total_size);
	printf("throughput %fMB/s\n", (double)total_size/TIME(st, et));
	while(j);

	return EXIT_SUCCESS;
}
