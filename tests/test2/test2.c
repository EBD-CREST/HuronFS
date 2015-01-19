#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>

//access to a remote file
//test function:
//open, read, write, flush, close

const size_t READ_SIZE=34;
int main(int argc, const char ** argv)
{
	int fd1, fd2;
	char *buffer=NULL;
	char *data="append this context after test1";
	if(-1 == (fd1=open("../../../test1", O_RDWR)))
	{
		perror("open");
		return EXIT_FAILURE;
	}
	if(-1 == (fd2=open("../../../test2",O_RDONLY)))
	{
		perror("open");
		return EXIT_FAILURE;
	}
	if(NULL == (buffer=malloc(35*sizeof(char))))
	{
		perror("malloc");
		return EXIT_FAILURE;
	}

	if(-1 == read(fd1, buffer, 34*sizeof(char)))
	{
		perror("read");
		return EXIT_FAILURE;
	}
	buffer[34]=0;
	printf("%s\n", buffer);
	if(-1 == read(fd2, buffer, sizeof(char)))
	{
		perror("read");
		return EXIT_FAILURE;
	}

	if(-1 == lseek(fd1, 38, SEEK_SET))
	{
		perror("lseek");
		return EXIT_FAILURE;
	}

	if(-1 == write(fd1, data, strlen(data)))
	{
		perror("write");
		return EXIT_FAILURE;
	}
	close(fd1);
	close(fd2);
	return EXIT_SUCCESS;
}
